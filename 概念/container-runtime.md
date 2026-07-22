---
title: Container Runtime
summary: Container Runtime 是负责运行容器的底层组件，包括 containerd、CRI-O 等实现。
category: concepts
tags:
- container-runtime
- cri
- core
- visibility/public
tier: core
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Container Runtime

## 概述

Container Runtime（容器运行时）是负责在节点上实际拉取镜像、创建、启动、停止容器的底层组件。Kubernetes 自 v1.24 起彻底移除内置的 dockershim，全面转向 **CRI（Container Runtime Interface）** 标准。这意味着 kubelet 不再直接对接 Docker，而是通过统一的 gRPC 接口与任何符合 CRI 的运行时交互。当前主流实现是 **containerd** 和 **CRI-O**，二者均基于 OCI（Open Container Initiative）规范。

## 架构与工作原理

```
┌─────────────────────── 节点 ───────────────────────┐
│  kubelet                                            │
│     │ CRI gRPC (/runtime.v1.RuntimeService)         │
│     ▼                                               │
│  ┌──────────── Container Runtime ────────────┐      │
│  │ 高层 (CRI Shim)                            │      │
│  │   containerd / CRI-O                       │      │
│  │     │ 镜像管理、卷、网络 (CNI) 调用          │      │
│  │     ▼                                       │      │
│  │ 低层 OCI Runtime                            │      │
│  │   runc / crun / kata-runtime               │      │
│  │     │ 创建 namespace + cgroup              │      │
│  │     ▼                                       │      │
│  │ 容器进程                                    │      │
│  └─────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────┘
```

**高层 vs 低层运行时**：
- **高层运行时（High-level）**：containerd、CRI-O、Docker Engine。负责镜像管理、卷、CRI 适配、调用 CNI。
- **低层运行时（Low-level）**：runc（默认）、crun（C 语言更轻）、kata-container（沙箱 VM）、gVisor（用户态内核）。负责真正创建 namespace、cgroup 并 `exec` 容器进程。

**Kubelet ↔ Runtime 交互**：kubelet 通过 unix socket（如 `/run/containerd/containerd.sock`）调用 CRI 接口；运行时负责 ImageService（拉镜像）和 RuntimeService（创建/启动 Pod 的 sandbox 与容器）。

## 关键组件与特性

| 组件 | 说明 |
|------|------|
| CRI | gRPC 接口规范，隔离 kubelet 与具体实现 |
| containerd | CNCF 毕业项目，Docker 拆分出的核心，业界默认 |
| CRI-O | Red Hat 主导，专为 Kubernetes 精简设计 |
| runc | OCI 参考实现，低层运行时事实标准 |
| CNI | 容器网络接口，由运行时调用为 Pod 配置网卡 |
| RuntimeClass | kubelet 资源，让不同 Pod 用不同运行时（如 kata 沙箱） |

## 配置示例

**kubelet 指定运行时**（节点 systemd 配置）：

```yaml
# /etc/kubernetes/kubelet.conf.d/10-containerd.conf
apiVersion: kubelet.config.k8s.io/v1
kind: CredentialProviderConfig
# kubelet 启动参数
# --container-runtime=remote
# --container-runtime-endpoint=unix:///run/containerd/containerd.sock
```

**containerd 配置**（`/etc/containerd/config.toml` 关键段）：

```toml
version = 2
[plugins."io.containerd.grpc.v1.cri"]
  # 沙箱镜像（Pause）
  sandbox_image = "registry.k8s.io/pause:3.9"
  # SystemdCgroup：与 kubelet cgroup driver 对齐
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
    SystemdCgroup = true
  # 私有仓库
  [plugins."io.containerd.grpc.v1.cri".registry.configs."registry.example.com".auth]
    username = "robot$reader"
    password_file = "/etc/containerd/registry-passwd"
  # 镜像拉取并发与重试
  [plugins."io.containerd.grpc.v1.cri".registry]
    config_path = "/etc/containerd/certs.d"
```

**RuntimeClass：让部分 Pod 跑在沙箱运行时**：

```yaml
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
---
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-job
spec:
  runtimeClassName: kata
  containers:
  - name: app
    image: untrusted/binary:v1
```

## 常用操作与命令

```bash
# 查看节点上的运行时
kubectl get nodes -o wide   # 看 CONTAINER-RUNTIME 列

# containerd CLI（crictl 兼容 CRI）
crictl ps -a                 # 列出容器
crictl pods                  # 列出 Pod sandbox
crictl images                # 列出镜像
crictl logs <container-id>
crictl exec -it <id> -- /bin/sh
crictl stats

# containerd 自身管理（ctr / nerdctl）
ctr -n k8s.io containers list
nerdctl --namespace k8s.io ps

# 镜像清理（磁盘紧张时）
crictl rmi --prune
nerdctl image prune -f

# 查看 runtime 与 cgroup driver 是否一致
kubectl describe node <node> | grep -i runtime
```

## 最佳实践

1. **统一 cgroup driver**：kubelet 与运行时都用 `systemd`，避免混用 cgroupfs 导致资源统计错乱。
2. **镜像仓库用镜像缓存**：配置 containerd 的 `config_path`（registry-mirror）加速拉取，降低出网成本。
3. **RuntimeClass 隔离不可信负载**：多租户或运行第三方代码时用 gVisor/kata 提供更强隔离。
4. **沙箱 pause 镜像版本对齐**：kubelet 与 containerd 的 sandbox_image 版本不一致会报警告。
5. **预留镜像 GC 阈值**：通过 kubelet `--image-gc-high-threshold` / `--image-gc-low-threshold` 防止节点磁盘被打满。

## 常见陷阱

- **cgroup driver 不一致**：kubelet systemd、容器运行时 cgroupfs → Pod 资源统计异常、CPU limit 不生效。
- **镜像拉取超时**：跨地域仓库未配镜像加速；containerd 1.7+ 用 certs.d 目录做 endpoint 覆盖。
- **CNI 未就绪导致 Pod 卡在 ContainerCreating**：节点上 CNI 插件二进制或配置缺失，检查 `/etc/cni/net.d/`。
- **RuntimeClass handler 不存在**：声明了 `runtimeClassName: kata` 但节点没装 kata-runtime，Pod 一直 Pending。
- **磁盘写满触发 GC 误删**：低阈值设得太低，频繁 GC 影响启动延迟，建议 high=85%、low=80%。
- **Docker 兼容性误区**：v1.24+ 不再有 dockershim，遗留集群需迁移至 containerd（可用 `crictl` 替代 `docker` 命令）。

## 源码实现分析

### containerd CRI 插件架构

```go
// github.com/containerd/containerd/pkg/cri/server/service.go
// containerd CRI 插件：实现 K8s CRI 接口
func (c *criService) RunPodSandbox(ctx context.Context, r *runtime.RunPodSandboxRequest) {
    config := r.GetConfig()
    // 1. 创建 sandbox 容器（pause 容器）
    sandbox := c.createSandboxContainer(ctx, config)
    // 2. 调用 CNI 设置网络命名空间
    c.setupPodNetwork(ctx, sandbox, config.GetMetadata().GetNamespace())
    // 3. 创建并启动 task
    task, _ := sandbox.NewTask(ctx, cio.NewCreator(cio.WithStdio))
    task.Start(ctx)
    // 4. 返回 sandbox ID
    return &runtime.RunPodSandboxResponse{PodSandboxId: sandbox.ID()}
}

func (c *criService) CreateContainer(ctx context.Context, r *runtime.CreateContainerRequest) {
    // 1. 获取 sandbox
    sandbox := c.getSandbox(r.GetPodSandboxId())
    // 2. 拉取镜像（如果本地没有）
    image := c.ensureImage(ctx, config.GetImage().GetImage())
    // 3. 创建容器（配置 OCI spec）
    container, _ := c.client.NewContainer(ctx, id,
        containerd.WithImage(image),
        containerd.WithSpec(ociSpec),  // OCI runtime spec
        containerd.WithRuntime("io.containerd.runc.v2"),
    )
    return &runtime.CreateContainerResponse{ContainerId: id}
}
```

### 容器运行时架构对比

```
┌───────────────────────────────────────────────────────────┐
│          容器运行时架构对比                          │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  kubelet                                                  │
│    │  CRI gRPC                                           │
│    ▼                                                      │
│  containerd / CRI-O  (高级运行时)                      │
│    │  镜像管理 + 容器生命周期 + CNI 调用             │
│    ▼                                                      │
│  runc / kata / gVisor  (低级运行时)                   │
│    │  实际创建容器进程                              │
│    ▼                                                      │
│  Linux Kernel                                             │
│    ├─ namespaces (PID/NET/MNT/UTS/IPC/USER)            │
│    ├─ cgroups v2 (CPU/Memory/IO)                       │
│    ├─ seccomp-bpf (系统调用过滤)                     │
│    └─ capabilities (权限控制)                          │
│                                                           │
│  运行时对比:                                             │
│  runc:    共享内核，轻量，标准隔离                  │
│  kata:    独立内核(microVM)，强隔离，开销较大      │
│  gVisor:  用户态内核，中等隔离，syscall 受限      │
└───────────────────────────────────────────────────────────┘
```

### RuntimeClass 使用示例（🟡 部署到集群）

```yaml
# 定义 RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata  # 对应 containerd config 中的 runtime handler
scheduling:
  nodeSelector:
    katacontainers.io/kata-runtime: "true"
---
# 使用 RuntimeClass 运行强隔离 Pod
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-workload
spec:
  runtimeClassName: kata  # 使用 Kata microVM 隔离
  containers:
  - name: app
    image: untrusted-app:latest
    resources:
      limits:
        cpu: "2"
        memory: 4Gi
```

## 面试要点

1. **CRI 架构中高级运行时和低级运行时的区别？**
   - 高级（containerd/CRI-O）：镜像管理、容器生命周期、CNI 调用
   - 低级（runc/kata/gVisor）：实际创建容器进程、内核交互
   - kubelet 通过 CRI gRPC 与高级运行时通信

2. **containerd vs CRI-O 如何选型？**
   - containerd：生态更广、Docker 兼容、插件丰富
   - CRI-O：专为 K8s 设计、更轻量、Red Hat 支持
   - 生产环境两者都成熟，按团队熟悉度选择

3. **runc/kata/gVisor 的隔离级别对比？**
   - runc：共享内核，namespace+cgroup 隔离，开销最小
   - kata：独立 microVM 内核，强隔离，~100ms 启动
   - gVisor：用户态内核，中等隔离，syscall 兼容性有限

4. **dockershim 移除的影响和迁移？**
   - K8s 1.24 移除 dockershim，不再原生支持 Docker
   - 迁移：切换到 containerd（Docker 底层也是 containerd）
   - 影响：kubectl 操作无变化，只是节点运行时配置变更

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/pods.md|Pod]] — 运行时承载的工作负载单元
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
