# Resource Management for Pods and Containers

## 概述

在 Kubernetes 中，你可以为 Pod 中的每个容器指定所需的资源量。最常见的资源类型是 CPU 和内存（RAM）。通过设置 `requests`（请求）和 `limits`（限制），调度器可以为 Pod 选择合适的节点，而 kubelet 则确保运行中的容器不会超出设定的资源上限。

## 核心概念/原理

### Requests 与 Limits

- **Requests（请求）**：表示容器正常运行所需的最低资源量。kube-scheduler 根据容器的 requests 总和来决定将 Pod 调度到哪个节点上。kubelet 也会为该容器预留至少请求量的资源。
- **Limits（限制）**：表示容器允许使用的最大资源量，由 kubelet 和容器运行时通过内核机制强制执行。

### 资源类型

- **CPU**：计算处理资源，单位为 Kubernetes CPU（1 CPU = 1 个物理核心或虚拟核心）。支持小数，例如 `500m` 表示 0.5 CPU。最小精度为 `1m` 或 `0.001` CPU。
- **Memory**：内存资源，单位为字节。支持以下后缀：`E, P, T, G, M, k` 或对应的二进制后缀 `Ei, Pi, Ti, Gi, Mi, Ki`。
- **Huge Pages**：Linux 特有的超大页内存资源，例如 `hugepages-2Mi`。
- **Extended Resources**：扩展资源，如 GPU、FPGA 等，需由集群管理员预先在节点上通告。

### Pod 级别资源（v1.34 Beta，默认启用）

通过启用 `PodLevelResources` 特性门控，可以为整个 Pod 设置统一的 CPU 和内存 requests/limits。这尤其适用于包含大量容器的 Pod，便于声明整体资源预算，并允许容器之间共享闲置资源。

## 关键机制或特性

### CPU 限制 enforcement

- CPU 限制通过 **CPU 节流（throttling）** 实现。当容器接近其 CPU limit 时，内核会限制该容器对 CPU 的访问。CPU limit 是硬性限制，容器无法使用超过其 limit 的 CPU。

### Memory 限制 enforcement

- 内存限制通过 Linux 内核的 **OOM Killer** 实现。当容器使用的内存超过其 limit 时，内核可能在检测到内存压力时终止该容器。因此，内存限制是**反应式**的，容器可能在被杀死前短暂超出 limit。
- **注意**：如果节点整体内存不足，即使容器未超过其 memory limit，但超过了其 memory request，该 Pod 也有可能被驱逐（Evicted）。

### 调度与容量

- 调度器确保对于每种资源类型，已调度容器的 requests 总和不超过节点的容量（Capacity）。
- 节点的 `.status.allocatable` 字段描述了可供 Pod 使用的实际资源量（已扣除系统守护进程预留部分）。

### 本地临时存储（Local Ephemeral Storage）

- kubelet 可以测量 Pod 使用的本地临时存储，包括 `emptyDir` 卷（非内存 backed）、容器日志、`/var/lib/kubelet` 中的可写层等。
- 内存 backed 的 `emptyDir` 卷（`emptyDir.medium: Memory`）会计入容器的内存使用，而不是本地临时存储。

### 原地 Pod 垂直扩缩容（In-Place Pod Resize，v1.35 Stable）

- 无需重新创建 Pod，即可通过 Pod 的 `/resize` 子资源修改容器的 CPU 和 memory requests/limits。
- 可通过 `resizePolicy` 控制容器是否需要重启。

### 扩展资源（Extended Resources）

- **节点级扩展资源**：通过 PATCH 节点 `status.capacity` 通告，例如 `example.com/foo`。
- **集群级扩展资源**：通常由调度器扩展器（scheduler extender）管理。
- 扩展资源的请求量必须为整数（如 `3`、`3000m`、`3Ki`），不支持小数（如 `0.5`）。

## 使用场景

- **多租户集群资源隔离**：通过为每个容器设置 requests 和 limits，确保不同工作负载公平使用节点资源。
- **保障关键应用 SLA**：为关键业务容器设置充足的 memory request 和 limit，避免在资源紧张时被 OOM Kill 或驱逐。
- **GPU/FPGA 调度**：通过扩展资源请求，将需要硬件加速的 Pod 调度到具备相应设备的节点上。
- **动态调整资源**：根据实际负载通过原地resize或 VPA（Vertical Pod Autoscaler）自动调整资源配置，优化成本和性能。

## 最佳实践/注意事项

- **始终设置 memory limits**：未设置 memory limit 的 Pod 可能耗尽节点内存，触发系统 OOM，影响节点上所有 Pod 的稳定性。
- **requests 应反映实际平均使用量**：合理的 requests 有助于调度器做出更优的放置决策，并减少资源碎片。
- **limits 应覆盖峰值使用量**：limits 应略高于正常峰值，以避免频繁的 CPU 节流或 OOM Kill。
- **小心内存 backed emptyDir**：内存 backed 的 `emptyDir` 会占用 Pod 的内存 limit，大量或过大的 emptyDir 可能导致节点内存耗尽。
- **使用 ResourceQuota 和 LimitRange**：在命名空间级别限制总资源消耗和默认资源配额，防止单个团队或应用过度使用资源。
- **区分 CPU 和内存的行为差异**：CPU 是**可压缩资源**（超限时节流），内存是**不可压缩资源**（超限时可能被 Kill），因此内存配置应更加保守。

## 参考链接

- [Kubernetes 官方文档 - Resource Management for Pods and Containers](https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/)
