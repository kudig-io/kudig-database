# Pod Overhead

## 概述

Pod Overhead（Pod 开销）是 Kubernetes 中一种用于核算 Pod 基础设施所消耗系统资源的方式。这些资源是容器内部运行所需资源之外的额外开销。Pod 的开销在准入时根据 Pod 的 RuntimeClass 相关联的开销进行设置。

## 核心概念/原理

当在节点上运行 Pod 时，Pod 本身会占用一定量的系统资源。在 Kubernetes 中，Pod Overhead 用于在容器请求和限制之外，额外计入 Pod 基础设施消耗的资源。

Pod 的 overhead 在调度 Pod 时会被考虑在内：调度器会将 Pod 的 overhead 与容器资源请求之和一起计算。同样，kubelet 在调整 Pod cgroup 大小以及执行 Pod 驱逐排序时也会包含 Pod overhead。

## 关键机制或特性

- **RuntimeClass 配置**：需要使用定义了 `overhead` 字段的 `RuntimeClass`。
- **准入控制器修改**：RuntimeClass 准入控制器会在准入时更新工作负载的 PodSpec，加入 `overhead` 字段。如果 PodSpec 已经定义了该字段，Pod 将被拒绝。
- **资源配额计算**：如果定义了 ResourceQuota，容器请求之和以及 `overhead` 字段都会被计入。
- **调度考虑**：调度器在决定哪个节点应该运行新 Pod 时，会将 Pod 的 `overhead` 与容器请求之和相加。
- **cgroup 限制**：kubelet 设置 Pod cgroup 的上限时，会基于容器限制之和加上 PodSpec 中定义的 `overhead`。
- **CPU shares**：对于 Guaranteed 或 Burstable QoS 的 Pod，kubelet 会根据容器请求之和加上 `overhead` 来设置 `cpu.shares`。
- **可观测性**：kube-state-metrics 提供了 `kube_pod_overhead_*` 指标来帮助识别 Pod overhead 的使用情况。

## 使用场景

- 使用虚拟化容器运行时（如 Kata Containers 结合 Firecracker）时，每个 Pod 需要为虚拟机和客户操作系统预留额外资源（如 120MiB 内存、250m CPU）。
- 需要精确计算节点资源使用情况，确保调度决策和 cgroup 限制都包含 Pod 级别的基础设施开销。

## 最佳实践/注意事项

- 确保使用的 RuntimeClass 正确定义了 `overhead` 字段。
- PodSpec 中不应预先定义 `overhead` 字段，否则会被准入控制器拒绝。
- 使用 `kubectl get pod <name> -o jsonpath='{.spec.overhead}'` 可以检查 Pod 的 overhead 值。
- 在节点描述中观察到的资源请求会包含 Pod overhead，这是预期的行为。

## 参考链接

- [Kubernetes 官方文档 - Pod Overhead](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-overhead/)
