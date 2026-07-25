---
title: 容器运行时与 CRI 集成源码分析
description: 基于 kubernetes-1.36.2 cri-api/cri-client 与 containerd-2.3.3 真实源码的 CRI 协议剖析，containerd/CRI-O 架构对比与 kubelet 到 runc 的完整调用链
summary: 从 CRI proto 定义与 kubelet 的 cri-client 出发，结合 containerd-2.3.3 CRI plugin 服务端源码（两侧行号均实测），拆解 kubelet→containerd/CRI-O→runc 的完整创建链、RuntimeClass 多运行时机制与镜像拉取路径，给出运行时层排障方法。
category: source-analysis
tags:
- k8s
- source-code
- cri
- containerd
- cri-o
- runc
- runtime
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- CRI 协议如何定义
- kubelet 与 containerd 如何交互
- containerd 与 CRI-O 架构差异
- 容器创建从 kubelet 到 runc 的完整链路
trigger_keywords:
- CRI
- containerd
- CRI-O
- runc
- shim
- RuntimeClass
- 容器运行时
related_domains:
- 容器运行时
- 集群基础
- 工作负载
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# 容器运行时与 CRI 集成源码分析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/staging/src/k8s.io/{cri-api,cri-client}/`（K8s 侧）+ `33-源码/容器运行时/containerd-2.3.3/`（运行时侧），两侧行号均实测；CRI-O 侧为机制级分析（源码树待入库，见 [[33-源码/README.md|33-源码 待补充清单]]）
> 本篇属 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 一、CRI：kubelet 与运行时的唯一契约

```protobuf
// staging/src/k8s.io/cri-api/pkg/apis/runtime/v1/api.proto（实测行号）
service RuntimeService {                     // :24
    rpc RunPodSandbox(...)     {}            // :30  建 Pod 沙箱（网络在此就绪）
    rpc CreateContainer(...)   {}            // :66  建容器（不启动）
    ...  StartContainer / StopContainer / ListContainers / Exec / Attach
}
service ImageService {                       // :211
    rpc PullImage(...)         {}            // :232 拉镜像（凭证由 kubelet 下发）
}
```

kubelet 侧的 gRPC 客户端封装：

```go
// staging/src/k8s.io/cri-client/pkg/remote_runtime.go（实测行号）
func NewRemoteRuntimeService(ctx, endpoint, connectionTimeout, tp, useStreaming)  // :95  连 unix socket
func (r *remoteRuntimeService) RunPodSandbox(ctx, config, runtimeHandler)         // :220
```

要点：

- 连接目标是 `--container-runtime-endpoint`（默认 `unix:///run/containerd/containerd.sock`），**kubelet 与运行时同机、无网络跳数**
- `runtimeHandler` 参数即 RuntimeClass 的落点：一个节点可同时提供 runc/gVisor/Kata 等多个 handler，Pod 用 `runtimeClassName` 选择
- 沙箱（sandbox）= pause 容器 + 网络命名空间：`RunPodSandbox` 返回前 CNI 已执行完毕（由运行时调用，见 [[10-平台工程/06-代码分析/kubernetes-ecosystem/02-cni-network-plugins.md|02 篇]]），因此「Pod 有 IP」严格早于「业务容器启动」

调用时序（对接 [[10-平台工程/06-代码分析/kubernetes-core/08-kubelet-deep-dive.md|kubelet 篇]]第二节的 SyncPod 顺序）：

```
kuberuntime.SyncPod(:1450)
  → RunPodSandbox        # pause 容器 + netns + CNI ADD
  → PullImage            # ImageService, 带 imagePullSecrets 凭证
  → CreateContainer      # 写 OCI spec（挂载、env、cgroup 参数）
  → StartContainer       # 交给 shim → runc
```

## 二、containerd：事实标准运行时的内部结构

```
kubelet ──CRI gRPC──▶ containerd
                        ├── CRI plugin（内置，实现 RuntimeService/ImageService）
                        ├── content/snapshotter（镜像层存储，默认 overlayfs）
                        ├── task service
                        │      └── containerd-shim-runc-v2（每 Pod 一个 shim 进程）
                        │              └── runc（创建后即退出，容器进程挂在 shim 下）
                        └── CNI 调用（libcni，RunPodSandbox 期间）
```

### CRI plugin 服务端实现（containerd-2.3.3，行号实测）

gRPC 服务注册与服务实例化：

```go
// plugins/cri/cri.go
runtime.RegisterRuntimeServiceServer(s, instrumented)   // :189 CRI 服务挂到 containerd 主 gRPC server
// internal/cri/server/service.go
func NewCRIService(options *CRIServiceOptions) (CRIService, runtime.RuntimeServiceServer, error)  // :193
```

四个核心 RPC 的服务端落点（与第一节 kubelet 侧调用时序逐一对应）：

```go
// internal/cri/server/sandbox_run.go
func (c *criService) RunPodSandbox(ctx, r)          // :54  建沙箱主入口
func (c *criService) setupPodNetwork(ctx, sandbox)  // :449 CNI 调用点本体
    netPlugin.SetupSerially / netPlugin.Setup       // :494/:496 → libcni AddNetworkList（02 篇 :515）

// internal/cri/server/images/image_pull.go
func (c *CRIImageService) PullImage(ctx, name, credentials, sandboxConfig, runtimeHandler)  // :123

// internal/cri/server/container_create.go
func (c *criService) CreateContainer(ctx, r)        // :60  CRI 请求 → OCI spec（不启动）

// internal/cri/server/container_start.go
func (c *criService) StartContainer(ctx, r)         // :50
    container.NewTask(ctx, ioCreation, taskOpts...) // :206 创建 task（拉起 shim）
    task.Start(ctx)                                 // :247 shim → runc start
```

销毁侧镜像对称：`sandbox_stop.go` StopPodSandbox:35 内调 teardownPodNetwork:154（CNI DEL），`sandbox_remove.go` RemovePodSandbox:34 清理沙箱元数据——**CNI DEL 发生在 StopPodSandbox 而非 Remove**，排查 IP 泄漏时应盯 Stop 路径。

从 :449/:496 可读出两个关键事实：**CNI 由 containerd 调用（非 kubelet）**；`netPlugin[runtimeClass]` 查表（:439）意味着不同 RuntimeClass 可配不同 CNI 配置。

架构上的三个生产要点：

1. **shim 进程是解耦关键**：containerd 重启不影响运行中容器（容器父进程是 shim 而非 containerd）；节点上 `ps` 看到的 `containerd-shim-runc-v2` 数 ≈ Pod 数，shim 泄漏（Pod 已删 shim 仍在）是节点进程数暴涨的常见原因
2. **snapshotter 决定镜像落盘形态**：overlayfs 目录在 `/var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/`，镜像 GC（kubelet imageGC 阈值触发）最终落到 content store 的引用计数删除
3. **配置入口 `/etc/containerd/config.toml`**：`SystemdCgroup = true` 必须与 kubelet `cgroupDriver: systemd` 一致——不一致时容器能起但 cgroup 统计错乱、驱逐失准，这是装机后最高频的坑

## 三、CRI-O 与运行时选型对比

| | containerd | CRI-O |
|---|-----------|-------|
| 定位 | 通用容器运行时（Docker 也基于它） | 专为 K8s 而生，版本随 K8s 同步发布 |
| CRI 实现 | 内置 CRI plugin | 整个项目就是 CRI 实现 |
| 每容器进程 | shim-runc-v2 | conmon（更轻，C 语言） |
| 镜像管理 | content store + snapshotter | containers/storage + containers/image 库 |
| 生态 | nerdctl/ctr 工具链、Docker 兼容 | OpenShift 默认、Podman 同库系 |

两者对上层完全等价（同一 CRI 契约），排障工具统一用 **crictl**（直接对 CRI socket 说话，与 kubelet 视角一致）：

```bash
# 🟢 低风险：只读
crictl pods && crictl ps -a && crictl inspect <cid>
crictl images && crictl imagefsinfo
```

## 四、OCI 边界：runc 与安全容器

CRI 之下还有一层 OCI 契约（runtime-spec/image-spec）：`CreateContainer` 时运行时把 CRI 请求翻译为 OCI Runtime Spec（config.json），交给 OCI 运行时执行：

- **runc**：默认实现，namespace+cgroup 直接跑在宿主内核
- **gVisor(runsc)**：用户态内核拦截 syscall，牺牲兼容性/性能换隔离
- **Kata**：轻量 VM 级隔离，每 Pod 一个 microVM
- RuntimeClass 把选择权暴露到 Pod 级，配合 `scheduling.nodeSelector`（RuntimeClass 字段）保证调度到具备该 handler 的节点——多租户安全边界设计见 [[08-安全/03-运行时安全/index.md|安全域：运行时安全]]

## 五、生产排障速查

| 症状 | 层次定位 | 检查手段 |
|------|---------|---------|
| Pod 卡 ContainerCreating | RunPodSandbox 失败（sandbox_run.go:54，常为 CNI/镜像） | `kubectl describe pod` 事件 + `crictl pods` + containerd 日志 |
| ImagePullBackOff | ImageService.PullImage（服务端 image_pull.go:123） | `crictl pull` 手工复现、凭证/仓库连通性（[[10-平台工程/06-代码分析/kubernetes-ecosystem/07-registry-dns-loadbalancer.md|07 篇 Harbor 节]]） |
| PLEG is not healthy | 运行时 List 接口慢 | `crictl ps` 耗时、containerd goroutine dump |
| 容器起但 cgroup 统计错乱 | cgroup driver 不一致 | config.toml SystemdCgroup vs kubelet cgroupDriver |
| shim 进程泄漏 | task 清理失败 | 对比 shim 进程数与 `crictl pods` 数 |
| exec/logs 超时 | streaming server 路径 | kubelet 10250 反向通道（07 篇通信矩阵） |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/08-kubelet-deep-dive.md|kubernetes-core 08 - kubelet 源码深度剖析]]（CRI 调用方）
- [[10-平台工程/06-代码分析/kubernetes-ecosystem/02-cni-network-plugins.md|02 - CNI 网络插件集成]]（沙箱网络）
- [[01-集群基础/03-控制平面/21-container-runtime-deep-dive.md|控制平面：容器运行时 Deep Dive]]
- [[14-容器运行时/README.md|容器运行时域]]
- [[08-安全/03-运行时安全/index.md|安全域：运行时安全]]
