---
title: Container Runtime Comparison
description: Container Runtime Comparison — Kubernetes 生产运维知识库
summary: Container Runtime Comparison — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- containerd
- cri-o
- docker
- runtime
- cri
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Container Runtime Comparison 是什么
- 如何 Container Runtime Comparison
trigger_keywords:
- Container
- Runtime
- Comparison
prerequisites:
- kubectl-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Container Runtime Comparison

## Runtime Layering

| Layer | Name | Examples | Responsibility |
|-------|------|---------|----------------|
| High-level | Container Engine | Docker, Podman | Image management, CLI, networking |
| Mid-level | CRI Manager | containerd, CRI-O | Container lifecycle, image [[distribution\|distribution]] |
| Low-level | OCI Runtime | runc, crun, youki | Actual container process creation |

## Production Runtime Comparison

| Dimension | containerd | CRI-O | Docker |
|-----------|-----------|-------|--------|
| Architecture | Monolithic daemon + shim | Monolithic daemon + conmon | dockerd + containerd |
| CRI Compatible | Native (CRI plugin) | Native (designed for CRI) | Requires dockershim (removed) |
| OCI Runtime | runc, crun, kata | runc, crun, kata | runc (default) |
| Image Management | ctr, nerdctl | crictl, podman | docker CLI |
| Memory Usage | Low (~100MB RAM) | Lowest (~50MB RAM) | High (~300MB RAM) |
| Image Pull | Parallel pull | Parallel pull | Parallel pull |
| K8s Integration | Default since v1.24 | Red Hat/OpenShift default | Deprecated in v1.24 |
| Community Support | Broadest | OpenShift ecosystem | Broadest toolchain |
| Security | Rootless support | Rootless support | Rootless support |
| Debug Tools | ctr, crictl, nerdctl | crictl, podman | docker CLI |
| Best For | General K8s clusters | OpenShift / security-first | Development |

## OCI Runtime Options

| Runtime | Language | Characteristics | Use Case |
|---------|----------|----------------|----------|
| runc | Go | OCI reference, most widely used | Default choice |
| crun | C | Lighter, faster startup | Resource-constrained |
| youki | Rust | Memory safe, experimental | Security experiments |
| gVisor (runsc) | Go | Kernel isolation (sandbox) | Multi-tenant, security |
| Kata Containers | Go | Lightweight VM isolation | Strong isolation required |

## K8s CRI Evolution

```
# 🟢 低风险：只读/信息收集，通常无副作用
2014-2020: K8s uses dockershim to talk to Docker Engine
2020: dockershim deprecated (Docker-specific coupling)
2021: dockershim removed from kubelet source
2022+: K8s nodes use containerd or CRI-O directly
        Docker images remain compatible (OCI Image Spec)
```
## Production Recommendations

- **Standard K8s**: containerd (default, proven, well-supported)
- **OpenShift / Security-first**: CRI-O (minimal attack surface)
- **Multi-tenant isolation**: Kata Containers or gVisor as runtime class
- **Development**: Docker Desktop (convenience, tooling)
- **CI/CD build**: Docker BuildKit or Kaniko (image building)

## 源码实现分析

### CRI 接口调用链

```go
// kubelet 通过 CRI gRPC 接口与运行时交互
// kubernetes/pkg/kubelet/kuberuntime/kuberuntime_manager.go
func (m *kubeGenericRuntimeManager) SyncPod(pod *v1.Pod, podStatus *kubecontainer.PodStatus) {
    // 1. 创建 Pod Sandbox（网络命名空间）
    sandboxID := m.runtimeService.RunPodSandbox(config)
    // CRI gRPC: RuntimeService.RunPodSandbox()
    //   → containerd: 创建 pause 容器 + CNI 网络配置
    
    // 2. 拉取镜像
    m.imageService.PullImage(imageSpec)
    // CRI gRPC: ImageService.PullImage()
    //   → containerd: 并行拉取层 + 解压
    
    // 3. 创建并启动容器
    containerID := m.runtimeService.CreateContainer(sandboxID, config)
    m.runtimeService.StartContainer(containerID)
    // CRI gRPC → containerd shim → runc create/start
}
```

### containerd 架构分层

```
┌─────────────────────────────────────────────────┐
│  kubelet (CRI gRPC client)                     │
└─────────────────┬───────────────────────────────┘
                  │ /run/containerd/containerd.sock
                  ▼
┌─────────────────────────────────────────────────┐
│  containerd (daemon)                            │
│  ├── CRI Plugin (实现 CRI 接口)                │
│  ├── Image Service (拉取/解压/存储)          │
│  ├── Snapshot Service (overlayfs 层管理)     │
│  └── Task Service (容器生命周期)             │
└─────────────────┬───────────────────────────────┘
                  │ shim v2 API
                  ▼
┌─────────────────────────────────────────────────┐
│  containerd-shim-runc-v2 (每容器一个)          │
│  └── runc (OCI runtime)                        │
│      ├── clone() + namespaces                  │
│      ├── cgroups 配置                          │
│      └── exec 容器进程                        │
└─────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：检查节点运行时状态

```bash
# 🟢 低风险 - 查看节点容器运行时
kubectl get nodes -o wide  # CONTAINER-RUNTIME 列

# 🟢 低风险 - 通过 crictl 检查容器
kubectl debug node/worker-1 -it --image=busybox -- chroot /host crictl ps

# 🟢 低风险 - 查看运行时版本
kubectl get nodes -o jsonpath='{.items[*].status.nodeInfo.containerRuntimeVersion}'
```

### 场景二：配置 RuntimeClass（安全隔离）

```yaml
# 使用 gVisor 沙箱运行不可信工作负载
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc    # containerd 配置的 runtime handler
---
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-app
spec:
  runtimeClassName: gvisor  # 使用 gVisor 沙箱
  containers:
  - name: app
    image: user-submitted:v1
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 移除 Docker 后镜像不兼容 | OCI 镜像标准统一，Docker 构建的镜像在 containerd 上完全兼容 |
| containerd 没有 CLI 工具 | nerdctl 提供与 docker 兼容的 CLI，crictl 用于调试 |
| CRI-O 只能用于 OpenShift | CRI-O 可用于任何 K8s 集群，只是 Red Hat 默认选择 |
| runc 是唯一的 OCI 运行时 | crun(C)、youki(Rust)、kata(VM)、gVisor(沙箱) 都是 OCI 运行时 |
| dockershim 移除影响开发 | 只影响 kubelet 与 Docker 的集成，本地开发仍可用 Docker |
| 容器就是轻量级虚拟机 | 容器是进程隔离（namespace+cgroup），无独立内核，与 VM 本质不同 |

## 面试要点

1. **为什么 K8s 移除了 dockershim？** — Docker 不原生支持 CRI，需要 dockershim 做转换层，增加复杂度和维护成本。containerd/CRI-O 原生实现 CRI，调用链更短（减少一层 daemon），性能更好、攻击面更小。

2. **containerd 与 CRI-O 如何选择？** — containerd：生态最广、社区最大、默认选择；CRI-O：更轻量（~50MB）、专为 K8s 设计、OpenShift 默认。功能上几乎等价，选择取决于生态偏好。

3. **RuntimeClass 的作用？** — 允许不同 Pod 使用不同的 OCI 运行时。典型场景：普通工作负载用 runc，不可信代码用 gVisor/runsc，强隔离需求用 Kata Containers。通过 handler 名称映射到 containerd 配置的 runtime。

4. **容器启动的完整链路？** — kubelet CRI gRPC → containerd CRI Plugin → 创建 sandbox(pause) → CNI 配置网络 → 拉取镜像 → 创建容器 → containerd-shim → runc create(创建 namespace/cgroup) → runc start(exec 进程)。

## Related

- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[youki]] — youki
- [[cri-o]] — CRI-O
- [[22-概念/15-运行时与系统/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[22-概念/15-运行时与系统/docker-architecture.md|Docker Architecture]]
- [[22-概念/15-运行时与系统/linux-container-foundation.md|Linux Container Foundation]]
- [[containerd|containerd]]
- [[cri-o|CRI-O]]
- OCI Standard


<!-- risk-assessed -->
