# Node-pressure Eviction

## 概述

节点压力驱逐（Node-pressure Eviction）是 kubelet 主动终止 Pod 以回收节点资源的过程。kubelet 监控节点的内存、磁盘空间、文件系统 inode 和 PID 等资源，当某些资源达到特定消耗水平时，kubelet 会主动使一个或多个 Pod 失败来回收资源，防止饥饿。

## 核心概念/原理

### 驱逐信号与阈值

kubelet 使用驱逐信号（eviction signals）来做出驱逐决策，将信号与驱逐阈值（eviction thresholds）进行比较。

**驱逐信号**：

| 信号 | 描述 |
|------|------|
| `memory.available` | 节点容量减去工作集内存 |
| `nodefs.available` | 节点主文件系统可用空间 |
| `nodefs.inodesFree` | 节点主文件系统可用 inode |
| `imagefs.available` | 镜像文件系统可用空间 |
| `imagefs.inodesFree` | 镜像文件系统可用 inode |
| `containerfs.available` | 容器文件系统可用空间 |
| `containerfs.inodesFree` | 容器文件系统可用 inode |
| `pid.available` | 可用进程标识符数量 |

**阈值类型**：

- **Soft eviction thresholds**：与管理员指定的宽限期配对。宽限期结束后才驱逐 Pod。使用 `eviction-soft`、`eviction-soft-grace-period` 和 `eviction-max-pod-grace-period` 配置。
- **Hard eviction thresholds**：没有宽限期，达到阈值后立即终止 Pod。默认硬阈值包括：
  - `memory.available<100Mi`（Linux）/ `<500Mi`（Windows）
  - `nodefs.available<10%`
  - `imagefs.available<15%`
  - `nodefs.inodesFree<5%`（Linux）
  - `imagefs.inodesFree<5%`（Linux）

### 节点条件

kubelet 将驱逐信号映射为节点条件：

| 节点条件 | 对应信号 |
|----------|----------|
| `MemoryPressure` | `memory.available` |
| `DiskPressure` | 各种文件系统信号 |
| `PIDPressure` | `pid.available` |

### 资源回收顺序

kubelet 在驱逐用户 Pod 之前会先尝试回收节点级资源：

- 垃圾回收死掉的 Pod 和容器。
- 删除未使用的镜像。

### Pod 选择顺序

如果节点级资源回收不足以将信号降到阈值以下，kubelet 开始驱逐用户 Pod，排序依据：

1. Pod 的资源使用是否超过请求量
2. Pod 优先级
3. 资源使用相对于请求量的多少

因此驱逐顺序大致为：
1. `BestEffort` 或资源使用超过请求的 `Burstable` Pod（按优先级和超量程度排序）
2. 资源使用未超过请求的 `Guaranteed` 和 `Burstable` Pod（按优先级排序）

## 关键机制或特性

- **最小回收量（eviction-minimum-reclaim）**：可以配置每种资源的最小回收量，防止 kubelet 反复触发多次驱逐。
- **节点条件振荡保护**：`eviction-pressure-transition-period`（默认 5 分钟）控制 kubelet 在切换节点条件状态前必须等待的时间，防止条件快速振荡导致错误的驱逐决策。
- **OOM 行为**：如果 kubelet 无法在内核 OOM killer 之前回收内存，系统会依赖 OOM killer。kubelet 根据 Pod 的 QoS 为每个容器设置 `oom_score_adj` 值。
- **MergeDefaultEvictionSettings**：kubelet 配置中的此字段设为 true 时，修改某个阈值参数后其他参数会继承默认值而不是 0。

## 使用场景

- 防止节点因内存、磁盘或 inode 耗尽而导致系统不稳定。
- 在资源紧张时自动释放节点资源，保持节点健康。
- 配合 Pod 优先级和 QoS 类，实现有策略的资源回收。

## 最佳实践/注意事项

- 节点压力驱逐与 API-initiated 驱逐不同，kubelet **不尊重** PodDisruptionBudget 和 `terminationGracePeriodSeconds`。
- 软驱逐阈值可配置最大 Pod 优雅终止期；硬驱逐阈值使用 0 秒宽限期（立即关闭）。
- 配置驱逐策略时，应确保调度器不会调度会立即触发驱逐的 Pod。
- 如果不想 DaemonSet 的 Pod 被驱逐，应为它们设置足够高的优先级。
- 对于 Linux 节点，`memory.available` 的计算排除了 `inactive_file`，因为 kubelet 假设这部分内存可以在压力下回收。
- 大量使用本地存储的工作负载可能会因内核缓存被计为 `active_file` 而触发内存压力驱逐，可以通过将内存限制和请求设为相同值来缓解。

## 参考链接

- [Kubernetes 官方文档 - Node-pressure Eviction](https://kubernetes.io/docs/concepts/scheduling-eviction/node-pressure-eviction/)
