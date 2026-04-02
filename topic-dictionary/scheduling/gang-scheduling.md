# Gang Scheduling

## 概述

Gang Scheduling（组调度）确保一组 Pod 以"全有或全无"的方式进行调度。如果集群无法容纳整个组（或定义的最低数量），则组中没有任何 Pod 会被绑定到节点上。该特性在 Kubernetes v1.35 中为 alpha 状态。

## 核心概念/原理

Gang Scheduling 依赖于 Workload API。需要启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组。

当启用 `GangScheduling` 插件时，调度器会更改属于 Workload 中 `gang` pod group 策略的 Pod 的生命周期：

1. **PreEnqueue 阶段**：调度器会 hold 住 Pod，直到：
   - 引用的 Workload 对象已创建。
   - 引用的 pod group 存在于 Workload 中。
   - 为该特定组创建的 Pod 数量至少等于 `minCount`。
   
   在这些条件满足之前，Pod 不会进入活动调度队列。

2. **调度阶段**：一旦满足法定数量（quorum），调度器尝试为组中的所有 Pod 找到放置位置。所有已分配的 Pod 在此过程中会在 `WaitOnPermit` 门处等待。

3. **绑定阶段**：如果调度器能为至少 `minCount` 个 Pod 找到有效的放置位置，则允许所有找到的 Pod 绑定到其分配的节点。如果在 5 分钟的固定超时内无法为整个组找到放置位置，则不会调度任何 Pod，而是将它们移到不可调度队列中等待集群资源释放。

## 关键机制或特性

- **Workload API 依赖**：Gang Scheduling 的核心是 Workload API 中的 `gang` pod group 策略。
- **minCount**：定义了组调度的最低 Pod 数量要求。
- **WaitOnPermit**：Pod 在找到放置位置后但在绑定前会在此等待，确保整个组满足条件后才进行绑定。
- **超时机制**：固定 5 分钟超时，超时后 Pod 会被移到不可调度队列，让其他工作负载有机会被调度。
- **Alpha 限制**：在 alpha 阶段，找到放置位置是基于逐个 Pod 调度的方式，而非单周期方式。

## 使用场景

- 分布式训练作业（如 MPI、TensorFlow、PyTorch）需要所有工作进程同时启动，否则训练无法进行。
- 需要原子性调度的批处理作业，确保资源分配的一致性。
- 多 Pod 协作的应用场景，其中部分 Pod 无法单独运行。

## 最佳实践/注意事项

- 必须启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组。
- 由于当前是 alpha 特性，实现方式是基于逐个 Pod 调度，可能存在一定的调度延迟。
- 超时后 Pod 会进入不可调度队列，工作负载设计需要能容忍这种等待。

## 参考链接

- [Kubernetes 官方文档 - Gang Scheduling](https://kubernetes.io/docs/concepts/scheduling-eviction/gang-scheduling/)
