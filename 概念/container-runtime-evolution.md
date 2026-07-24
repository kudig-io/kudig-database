---
title: 容器运行时演进
summary: 容器运行时演进：containerd 是 CNCF 毕业项目，Kubernetes 最广泛采用的容器运行时。
category: concepts
tags:
- runtime
- containerd
- wasm
- confidential-containers
- k8s
tier: core
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
status: reviewed
---



# 容器运行时演进

## containerd 2.x

[[containerd]] 是 CNCF 毕业项目，Kubernetes 最广泛采用的容器运行时。

- **v2.0 LTS**：2024 年发布，移除 CRI v1alpha2，仅保留 CRI v1；引入 Sandbox API 支持可插拔沙箱运行时
- **v2.3 LTS**：保持约 4 个月发布节奏，持续优化稳定性与性能
- **CRI v1 唯一**：彻底废弃 v1alpha2 接口，简化代码路径
- **Sandbox API**：将 Pod sandbox 管理从 kubelet 解耦，支持 Kata Containers、WASM 等替代沙箱实现

## CRI-O

- 严格跟随 Kubernetes 版本发布（1.29、1.30…）
- 从 v1.29 起 crun 成为默认 OCI 运行时（替代 runc）
- 面向生产优化，Red Hat OpenShift 默认运行时

## crun vs runc

| 维度 | crun | runc |
|------|------|------|
| 语言 | C | Go |
| 启动速度 | 2-5x 更快 | 基准 |
| 内存占用 | ~1MB | ~10MB |
| WASM 支持 | 原生 | 无 |

crun 由 Red Hat 维护，适合低延迟与资源受限场景。

## User Namespaces GA（Kubernetes v1.36）

- 容器内 root 用户映射到宿主机非特权用户，大幅降低逃逸风险
- 支持 **idmap mounts**：文件权限正确映射，无需 chown
- 无需修改镜像或应用代码即可启用

参见 User Namespaces。

## WASM 容器

WebAssembly 作为新型容器运行时正在崛起：

- **runwasi**：containerd 的 WASM shim，使 containerd 可调度 WASM 工作负载
- **[[SpinKube]]** v0.24：CNCF Sandbox，将 Fermyon Spin 应用编排为 K8S 原生工作负载
- **sub-ms 冷启动**：WASM 模块启动延迟在亚毫秒级，远快于传统容器
- 适用于边缘计算、函数即服务、插件系统等场景

## 机密容器（Confidential Containers）

- **CoCo**：CNCF 沙箱项目，提供跨 TEE 厂商的统一机密容器标准
- **Kata Containers** 3.x：支持 Intel TDX、AMD SEV-SNP、ARM CCA 等 TEE 环境
- **GPU passthrough**：机密容器支持 GPU 直通，使 AI/ML 推理在 TEE 内安全执行
- 数据在使用中始终加密，解决多租户与公有云场景的信任根问题

参见 confidential-containers、TEE attestation。

## 镜像懒加载（Lazy Pulling）

传统容器启动需要完整拉取镜像，懒加载技术按需获取数据块：

- **Nydus** RAFS：CNCF 子项目，阿里开源，基于 FUSE/virtio-fs 的按需加载
- **eStargz**：Google 提出，兼容 OCI 标准的可寻址压缩格式
- **OverlayBD**：阿里云提出的块设备级懒加载方案

**效果**：冷启动时间缩短 50-80%，尤其在大镜像（>1GB）场景效果显著。

参见 OCI image spec、image lifecycle。

## 技术深度解析

### CRI 架构演进

容器运行时接口（CRI）的演进反映了 Kubernetes 解耦运行时的持续努力：

```
K8s 1.20 之前: Docker (dockershim) → 内置于 kubelet
K8s 1.20-1.24: dockershim 废弃警告 → CRI 过渡期
K8s 1.24+:     移除 dockershim → containerd / CRI-O 直接对接
K8s 1.32+:     CRI v1 唯一 → 移除 v1alpha2
```

### RuntimeClass 与多运行时共存

生产集群可以同时运行多种容器运行时，通过 RuntimeClass 按工作负载选择：

```yaml
# 标准容器（默认 runc）
spec:
  runtimeClassName: runc          # 高性能、低开销

# 安全隔离（Kata Containers）
spec:
  runtimeClassName: kata          # VM 级隔离，多租户场景

# WASM 工作负载
spec:
  runtimeClassName: wasm          # 亚毫秒启动、极小体积
```

### containerd 2.0 Sandbox API

containerd 2.0 引入的 Sandbox API 将 Pod sandbox 管理从 kubelet CRI 中解耦：

```
传统 CRI: kubelet → CRI RunPodSandbox → containerd 内部处理
Sandbox API: kubelet → CRI → containerd Sandbox Controller
  → 支持可插拔 sandbox 实现（runc / kata / wasm）
  → sandbox 生命周期独立于容器管理
```

## 最佳实践

- **containerd 作为默认运行时**：生产环境默认使用 containerd 2.x（CRI v1），无需 Docker——更轻量、更安全
- **安全敏感场景考虑 Kata Containers**：多租户或强隔离需求场景使用 Kata Containers 提供虚拟机级隔离，配合 Pod Overhead 计量资源
- **大镜像场景启用懒加载**：AI/ML 场景镜像通常 >1GB，启用 Nydus 或 eStargz 懒加载可将冷启动从分钟级降至秒级
- **ARM 节点使用 crun 替代 runc**：crun 在 ARM 平台上启动更快、内存占用更小——CRI-O 已默认使用 crun
- **为 WASM 工作负载准备独立节点池**：WASM shim 与传统容器行为不同，通过 RuntimeClass 和 taint 隔离 WASM 节点

## 常见陷阱

- **containerd 版本与 K8s 不兼容**：K8s 1.32+ 要求 containerd 2.0+，升级 K8s 前必须先升级运行时——否则 Pod 无法创建
- **镜像懒加载的首次访问延迟**：懒加载将镜像拉取延迟到运行时，首次文件访问可能变慢——不适合启动时加载大量文件的应用
- **机密容器性能开销**：TEE 加密/解密有 5-15% 性能损耗——需要评估是否值得安全收益

## 源码实现分析

### CRI 接口与 kubelet 集成

```go
// k8s.io/cri-api/pkg/apis/runtime/v1/api.pb.go
// CRI v1 接口定义（kubelet ↔ containerd/CRI-O）
type RuntimeServiceClient interface {
    // Pod Sandbox 管理
    RunPodSandbox(ctx context.Context, in *RunPodSandboxRequest) (*RunPodSandboxResponse, error)
    StopPodSandbox(ctx context.Context, in *StopPodSandboxRequest) (*StopPodSandboxResponse, error)
    // 容器生命周期
    CreateContainer(ctx context.Context, in *CreateContainerRequest) (*CreateContainerResponse, error)
    StartContainer(ctx context.Context, in *StartContainerRequest) (*StartContainerResponse, error)
    StopContainer(ctx context.Context, in *StopContainerRequest) (*StopContainerResponse, error)
    // 镜像管理
    PullImage(ctx context.Context, in *PullImageRequest) (*PullImageResponse, error)
    // 运行时状态
    Status(ctx context.Context, in *StatusRequest) (*StatusResponse, error)
}
// kubelet 通过 Unix Socket 连接 containerd:
// /run/containerd/containerd.sock (CRI v1)
```

### 容器运行时演进时间线

```
┌──────────────────────────────────────────────────────────┐
│            容器运行时演进时间线                        │
├──────────────────────────────────────────────────────────┤
│  2013 │ Docker 发布（单体架构）                       │
│  2015 │ OCI 标准成立（runtime/image/distribution）     │
│  2016 │ containerd/CRI-O 独立项目                     │
│  2017 │ CRI v1alpha1（kubelet 对接多运行时）           │
│  2020 │ K8s 宣布弃用 dockershim                      │
│  2022 │ K8s 1.24 移除 dockershim                     │
│  2023 │ containerd 2.0 + Sandbox API                  │
│  2024 │ K8s 1.32 CRI v1 唯一 + DRA beta              │
│  2025+│ WASM 运行时 / 机密容器 / 懒加载              │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：检查节点运行时版本

```bash
# 🟢 低风险：只读检查
kubectl get nodes -o wide  # 查看 CONTAINER-RUNTIME 列
crictl version  # 查看 CRI 版本
crictl info | jq '.config.containerd'  # containerd 配置
# 检查运行时健康状态
crictl stats  # 容器资源使用
crictl images  # 已拉取镜像
systemctl status containerd  # 服务状态
```

### 场景二：多运行时节点配置

```toml
# /etc/containerd/config.toml
# 🟡 中风险：修改后需重启 containerd
version = 2
[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"
  # runc 运行时（默认）
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true
  # Kata Containers（安全隔离）
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
  # WASM 运行时
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.wasm]
    runtime_type = "io.containerd.wasm.v1"
```

### 场景三：从 Docker 迁移到 containerd

```bash
# 🟠 高危：影响节点上所有容器
# 1. 排干节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
# 2. 停止 Docker
systemctl stop docker docker.socket containerd
# 3. 安装/配置 containerd
apt install containerd.io
containerd config default > /etc/containerd/config.toml
# 修改 SystemdCgroup = true
sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
# 4. 启动 containerd
systemctl enable --now containerd
# 5. 修改 kubelet 配置指向 containerd
# /var/lib/kubelet/kubeadm-flags.env:
# --container-runtime-endpoint=unix:///run/containerd/containerd.sock
systemctl restart kubelet
# 6. 恢复调度
kubectl uncordon <node>
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 移除 Docker 后不能用 Docker 命令 | nerdctl 提供完全兼容的 CLI；docker build 可用 BuildKit/kaniko 替代 |
| 2 | containerd 2.0 与 1.x 完全兼容 | 2.0 移除 v1alpha2 CRI、改变配置格式；升级需检查配置兼容性 |
| 3 | 所有节点必须用同一运行时 | RuntimeClass 允许不同节点池用不同运行时（runc/kata/wasm） |
| 4 | WASM 可以替代容器 | WASM 适合轻量级函数；复杂应用（多进程、特权操作）仍需容器 |
| 5 | 懒加载没有缺点 | 懒加载将拉取延迟转为运行时 I/O；启动时加载大量文件的应用反而更慢 |
| 6 | 升级 K8s 不需要升级运行时 | K8s 1.32+ 要求 containerd 2.0+；必须先升级运行时再升级 K8s |

## 面试要点

1. **Q: 容器运行时从 Docker 到 containerd 的演进原因是什么？**
   A: ① 架构简化：Docker 单体架构（dockerd→containerd→runc）冗余；直接使用 containerd 减少一层调用。② 标准化：CRI 接口让 kubelet 不依赖特定运行时。③ 维护成本：dockershim 在 kubelet 内部，Docker API 变化需同步适配。④ 性能：移除 dockershim 后 Pod 启动延迟降低 ~20%。⑤ 安全：减少攻击面（无需 dockerd 守护进程）。

2. **Q: containerd 2.0 Sandbox API 解决什么问题？**
   A: 传统 CRI 中 Pod sandbox 管理耦合在 kubelet 中，无法支持非传统 sandbox（如 Kata VM、WASM 实例）。Sandbox API 将 sandbox 生命周期抽象为独立控制器，支持可插拔 sandbox 实现。优势：① sandbox 创建/销毁独立于容器管理；② 支持自定义 sandbox 实现（机密容器、WASM）；③ 更好的资源计量（Pod Overhead）。

3. **Q: 生产环境如何选择和配置容器运行时？**
   A: ① 默认节点：containerd 2.x + runc（SystemdCgroup=true）；② 多租户/安全敏感：Kata Containers（VM 级隔离）+ RuntimeClass + Pod Overhead；③ 边缘/Serverless：WASM shim（亚毫秒启动）；④ ARM 节点：crun 替代 runc（更轻量）。关键：升级 K8s 前先升级运行时；监控运行时健康状态。

4. **Q: 镜像懒加载（Nydus/eStargz）的原理和适用场景？**
   A: 原理：将镜像层转为可按需读取的格式（Nydus 用 RAFS 文件系统，eStargz 用 gzip+TOC），容器启动时只拉取启动必需文件，其余按需从 registry 读取。适用：AI/ML 大镜像（>1GB）冷启动从分钟级降至秒级。不适用：启动时加载大量文件的应用（首次访问延迟高）。配合 P2P 分发（Dragonfly）效果更佳。

## 相关页面

- [[概念/specialized-k8s-technologies.md|K8S 专项技术]] — WASM 与边缘计算
- [[概念/k8s-security-compliance.md|K8S 安全与合规]] — 机密容器与 TEE
- [[概念/k8s-ai-ml-infrastructure.md|K8S AI/ML 基础设施]] — 大镜像优化
- [[概念/pod-overhead.md|Pod Overhead]] — 安全运行时的资源计量
- [[概念/kubernetes-containerd-integration.md|K8s 与 containerd 集成]] — CRI 通信架构
