---
title: Docker Architecture and Container Runtime
description: Docker Architecture and Container Runtime — Kubernetes 生产运维知识库
summary: Docker Architecture and Container Runtime — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- docker
- container
- containerd
- oci
- runtime
- kubelet
- cri-o
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Docker Architecture and Container Runtime 是什么
- 如何 Docker Architecture and Container Runtime
trigger_keywords:
- Docker
- Architecture
- and
- Container
- Runtime
prerequisites:
- kubectl-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Docker Architecture and Container Runtime

## Runtime Layered Architecture

Docker operates through five distinct layers, each communicating via standardized APIs:

| Layer | Name | Process | Interface |
|-------|------|---------|-----------|
| User Interface | Docker CLI | docker | REST API (Unix Socket) |
| API [[Service|Service]] | Docker Daemon | dockerd | [[gRPC|gRPC]] API |
| Container Manager | containerd | containerd | OCI Runtime Spec |
| Container Shim | containerd-shim | containerd-shim-runc-v2 | OCI Runtime |
| Low-level Runtime | runc | runc | Linux Kernel |

When `docker run nginx:latest` executes:
1. CLI sends request to dockerd via REST API over Unix Socket
2. dockerd delegates to containerd via gRPC
3. containerd spawns containerd-shim-runc-v2 process
4. shim calls runc to create the actual container
5. runc configures namespaces and cgroups, starts the container process
6. runc exits; shim takes over container lifecycle management

## OCI Standard

The OCI standard consists of three specifications:
- **Runtime Spec**: Container configuration, lifecycle, execution environment
- **Image Spec**: Image layers, config blob, manifest format
- **Distribution Spec**: Registry API, authentication, push/pull protocol

OCI ensures interoperability across container runtimes. Docker images (OCI format) run on any OCI-compliant runtime (runc, crun, youki, gVisor, Kata).

## Containerd Architecture

containerd provides a complete container management system with:
- **Content Store**: Content-addressable blob storage
- **Snapshots**: Filesystem snapshot management (overlayfs, btrfs, zfs)
- **Tasks**: Container process lifecycle
- **Namespaces**: Multi-tenant isolation
- **Leases**: Resource lifecycle management
- **Events**: Event streaming for monitoring

For K8s integration, containerd exposes the CRI (Container Runtime Interface) via the `io.containerd.grpc.v1.cri` plugin.

## K8s Runtime Evolution

```
# 🟢 低风险：只读/信息收集，通常无副作用
2014-2020: K8s uses dockershim (built-in Docker shim)
2020: K8s deprecates dockershim
2021: dockershim removed from kubelet
2022+: K8s nodes use containerd or CRI-O directly
       Docker images remain compatible (OCI standard)
```
Production nodes should use [[containerd|containerd]] or [[cri-o|CRI-O]] as the container runtime. Docker remains valuable for development and image building via BuildKit.

## Alternative Container Engines

| Engine | Daemonless | Rootless | K8s CRI | Best For |
|--------|-----------|----------|---------|----------|
| Docker | No | Limited | No (deprecated) | Development, build |
| Podman | Yes | Full | No | Security-sensitive dev |
| nerdctl | No | Yes | Yes | K8s nodes with Docker-like CLI |
| CRI-O | No | Limited | Yes (native) | K8s dedicated runtime |

## 源码实现分析

### containerd CRI 插件容器创建流程

```go
// containerd/pkg/cri/server/container_create.go
func (c *criService) CreateContainer(ctx context.Context, r *runtime.CreateContainerRequest) (*runtime.CreateContainerResponse, error) {
    // 1. 解析 Pod Sandbox 配置
    config := r.GetConfig()
    sandboxConfig := r.GetSandboxConfig()
    // 2. 生成 OCI 容器规格（namespaces, cgroups, mounts）
    spec, err := c.generateContainerSpec(config, sandboxConfig)
    // 3. 准备快照（overlayfs 层叠加）
    snapshotKey := config.Metadata.Name
    c.SnapshotService(snapshotter).Prepare(ctx, snapshotKey, parentChainID)
    // 4. 创建 containerd 容器对象
    container, err := c.client.NewContainer(ctx, id,
        containerd.WithSpec(spec),
        containerd.WithSnapshot(snapshotKey),
    )
    // 5. 创建 Task（实际调用 runc create）
    task, err := container.NewTask(ctx, cio.NewCreator(...))
    // 6. 启动容器进程（runc start）
    task.Start(ctx)
    return &runtime.CreateContainerResponse{ContainerId: id}, nil
}
```

### Docker 分层架构

```
┌──────────────────────────────────────────────────────────┐
│  docker run nginx:latest                                  │
├──────────────────────────────────────────────────────────┤
│  Docker CLI ──REST──▶ dockerd ──gRPC──▶ containerd       │
│                                              │            │
│                                    containerd-shim-runc-v2│
│                                              │            │
│                                           runc            │
│                                              │            │
│                              ┌────────────┼──────────┐  │
│                              │            │          │  │
│                         namespaces    cgroups    overlayfs │
│                         (pid/net/     (cpu/mem/  (layer   │
│                          mnt/uts)     io)       mount)   │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：生产节点 containerd 配置优化

```toml
# /etc/containerd/config.toml
# 🟡 中风险：修改后需重启 containerd
version = 2
[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true  # 生产必须启用，与 kubelet 一致
  [plugins."io.containerd.grpc.v1.cri".registry]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = ["https://mirror.internal:5000"]  # 内部镜像加速
```

### 场景二：调试容器运行时问题

```bash
# 🟢 低风险：只读诊断
crictl --runtime-endpoint unix:///run/containerd/containerd.sock ps -a  # 列出容器
crictl inspect <container-id> | jq '.status'  # 容器详细状态
crictl stats  # 容器资源使用
# 检查 runc 创建的 namespace/cgroup
ls /proc/<pid>/ns/  # 查看容器进程的 namespace
cat /sys/fs/cgroup/kubepods/pod<uid>/<container-id>/cpu.max  # cgroup v2 CPU 限制
# 检查 overlayfs 层叠加
mount | grep overlay  # 🟢 查看 overlay 挂载点
```

### 场景三：多运行时配置（安全容器）

```toml
# 🟡 中风险：添加新运行时需重启 containerd
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
  runtime_type = "io.containerd.kata.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata.options]
    ConfigPath = "/opt/kata/share/defaults/kata-containers/configuration.toml"
```
```yaml
# 通过 RuntimeClass 指定安全容器
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
---
apiVersion: v1
kind: Pod
spec:
  runtimeClassName: kata  # 使用 Kata Containers 强隔离
  containers:
  - name: untrusted-workload
    image: user-submitted-code:latest
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | K8s 移除 Docker 后不能用 Docker 镜像 | OCI 标准保证兼容；containerd/CRI-O 直接运行 Docker 格式镜像 |
| 2 | dockerd 是 K8s 必需的 | K8s 1.24+ 直接通过 CRI 对接 containerd/CRI-O，无需 dockerd |
| 3 | 容器是轻量级虚拟机 | 容器是进程隔离（namespace+cgroup），共享宿主机内核；VM 有独立内核 |
| 4 | 镜像层是复制关系 | 镜像层是 overlayfs 叠加（只读层+可写层），不是复制 |
| 5 | runc 是唯一的 OCI 运行时 | 还有 crun(C)、youki(Rust)、gVisor(用户态内核)、Kata(轻量VM) |
| 6 | containerd 和 CRI-O 功能相同 | containerd 通用容器管理（支持非 K8s 场景）；CRI-O 专为 K8s 设计，更精简 |

## 面试要点

1. **Q: 从 `docker run` 到容器进程启动，经历了哪些步骤？**
   A: ① CLI 发送 REST 请求到 dockerd；② dockerd 通过 gRPC 委托 containerd；③ containerd 拉取镜像、准备 overlayfs 快照；④ 启动 containerd-shim-runc-v2 进程；⑤ shim 调用 runc create（配置 namespace/cgroup）；⑥ runc start 启动容器进程；⑦ runc 退出，shim 接管生命周期（监控、日志、信号转发）。

2. **Q: 为什么 Kubernetes 移除了 dockershim？**
   A: dockershim 是历史产物（K8s 早期只支持 Docker）。移除原因：① 减少维护负担（dockershim 在 kubelet 内部，Docker API 变化需同步适配）；② 减少调用链路（Docker→containerd→runc 变为 containerd→runc）；③ Docker 不支持 CRI 标准，需要 shim 转换。移除后性能提升 ~20% Pod 启动延迟。

3. **Q: OCI 标准的三个规范分别解决什么问题？**
   A: ① Runtime Spec：定义容器配置、生命周期、执行环境（config.json 格式），确保任何运行时都能运行同一配置；② Image Spec：定义镜像层、config blob、manifest 格式，确保镜像跨运行时兼容；③ Distribution Spec：定义 Registry API、认证、push/pull 协议，确保镜像跨 registry 流通。

4. **Q: 生产环境如何选择容器运行时？**
   A: ① 通用 K8s 节点：containerd（最广泛、CNCF 毕业、生态完善）；② 安全敏感/多租户：Kata Containers（轻量 VM 隔离）或 gVisor（用户态内核）；③ 极简 K8s：CRI-O（专为 K8s 设计，攻击面小）；④ 开发环境：Docker/Podman（构建镜像、本地调试）。关键配置：必须启用 SystemdCgroup=true 与 kubelet 保持一致。

## Related

- [[22-概念/15-运行时与系统/container-runtime-comparison.md|container-runtime-comparison]] — Container Runtime Comparison
- [[docker]] — Docker
- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]] — Container Runtime
- [[containerd]] — containerd
- [[youki]] — youki
- [[22-概念/15-运行时与系统/linux-container-foundation.md|Linux Container Foundation]]
- [[22-概念/15-运行时与系统/container-runtime-comparison.md|Container Runtime Comparison]]
- [[22-概念/15-运行时与系统/overlayfs-storage.md|OverlayFS Storage]]
- [[containerd|containerd]]
- [[docker|Docker]]
- OCI Standard

- 01-docker-architecture-overview

<!-- risk-assessed -->
