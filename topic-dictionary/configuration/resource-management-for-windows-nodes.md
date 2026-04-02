# Resource Management for Windows nodes

## 概述

本文档概述了 Linux 与 Windows 节点在资源管理方面的差异。由于操作系统内核和进程隔离机制的不同，Kubernetes 在 Windows 节点上的资源管理方式与 Linux 存在显著区别。了解这些差异对于在混合操作系统集群中正确配置和调度工作负载至关重要。

## 核心概念/原理

### 进程隔离机制差异

- **Linux**：使用 `cgroups` 作为 Pod 边界进行资源控制，容器在该边界内创建以实现网络、进程和文件系统的隔离。Linux cgroup API 可用于收集 CPU、I/O 和内存使用统计信息。
- **Windows**：每个容器使用一个 **Job Object**（作业对象）配合系统命名空间过滤器来包含容器内的所有进程，并提供与主机的逻辑隔离。Job Object 是 Windows 的进程隔离机制，与 Kubernetes 中的 Job 工作负载概念不同。

### 权限与身份隔离

- Windows 容器运行时**必须启用命名空间过滤**，无法在主机上下文中声明系统特权。因此，Windows **不支持特权容器（privileged containers）**。
- 由于安全账户管理器（SAM）是独立的，容器无法假定主机的身份。

### 内存管理差异

- **无 OOM Killer**：Windows 没有像 Linux 那样的内存不足进程杀手。Windows 始终将所有用户模式内存分配视为虚拟内存，且必须使用页面文件（pagefiles）。
- **不超量使用内存**：Windows 节点不会为进程超量提交（overcommit）内存。当物理内存耗尽时，进程会通过页面文件换页到磁盘，而不会被 OOM 终止。
- **性能影响**：如果内存过度配置且所有物理内存耗尽，频繁的页面换入换出会显著降低性能。

### CPU 管理差异

- Windows 可以限制进程分配的 CPU 时间量，但**无法保证最小 CPU 时间**。
- kubelet 支持 `--windows-priorityclass` 命令行标志来设置 kubelet 进程的调度优先级，以确保 kubelet 不会被运行的 Pod 饿死 CPU 周期。建议设置为 `ABOVE_NORMAL_PRIORITY_CLASS` 或更高。

## 关键机制或特性

### 节点可分配资源（Node Allocatable）

- 为了计算操作系统、容器运行时和 Kubernetes 主机进程（如 kubelet）所占用的资源，应该使用 `--kube-reserved` 和/或 `--system-reserved` kubelet 标志来预留 CPU 和内存。
- 在 Windows 上，这些值仅用于计算节点的 **NodeAllocatable**，不会像 Linux 那样通过 cgroup 进行硬性限制。

### 调度与资源限制

- 部署工作负载时，应为容器设置内存和 CPU 的 limits。这些限制会从 NodeAllocatable 中扣除，帮助集群调度器决定将 Pod 放置到哪个节点上。
- **未设置 limits 的 Pod 可能导致 Windows 节点过度配置**，极端情况下会使节点变得不健康。

### 推荐的资源预留

- **内存**：在 Windows 节点上，建议至少预留 **2 GiB** 内存给系统开销。
- **CPU**：确定每个节点的最大 Pod 密度，并监控系统服务的 CPU 使用情况，然后根据工作负载需求选择合适的预留值。

## 使用场景

- **混合操作系统集群（Hybrid Cluster）**：在同时运行 Linux 和 Windows 工作节点的集群中，为 Windows 应用（如 .NET Framework、IIS）正确配置资源预留和限制。
- **Windows 容器化传统应用**：将运行在 Windows Server 上的遗留应用迁移到 Kubernetes 时，需要根据 Windows 的内存行为调整资源策略。
- **避免 Windows 节点性能衰退**：通过合理的资源预留和限制，防止 Windows 节点因过度配置导致严重的页面换页和响应延迟。

## 最佳实践/注意事项

- **务必设置资源 limits**：在 Windows 节点上调度 Pod 时，始终为容器设置 CPU 和内存 limits，避免调度器无法正确评估节点容量。
- **预留足够的系统内存**：至少为 Windows 系统组件和 kubelet 预留 2 GiB 内存。
- **设置 kubelet 优先级**：使用 `--windows-priorityclass=ABOVE_NORMAL_PRIORITY_CLASS` 或更高，防止 kubelet 被业务 Pod 饿死 CPU。
- **监控页面文件活动**：由于 Windows 依赖页面文件而非 OOM Killer，应持续监控磁盘 I/O 和页面文件使用率，作为内存压力的早期指标。
- **不要假设 Linux 的行为**：Windows 不会因为内存超限而杀死容器，而是性能下降；因此不能像 Linux 那样依赖 OOM Killer 来快速恢复。
- **合理评估 Pod 密度**：根据节点规格和系统开销，确定 Windows 节点上安全的最大 Pod 数量，避免资源争用。

## 参考链接

- [Kubernetes 官方文档 - Resource Management for Windows nodes](https://kubernetes.io/docs/concepts/configuration/windows-resource-management/)
