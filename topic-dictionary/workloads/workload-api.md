# Workload API

## 概述
Workload API 是 Kubernetes v1.35 引入的 Alpha 特性（默认禁用，需启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组）。它提供了一种结构化的、机器可读的多 Pod 应用调度需求定义，补充了现有工作负载控制器的运行时行为。

## 核心概念/原理
- **Workload 资源**：属于 `scheduling.k8s.io/v1alpha1` API 组，用于定义一组 Pod 的调度策略和放置约束。
- **与控制器分离**：Workload 资源决定 Pod 组应如何被调度，而 Job 等控制器决定运行什么。
- **结构组成**：
  - `podGroups`：定义工作负载的多个组件（如机器学习任务中的 driver 和 worker）。
  - `controllerRef`：链接到上层控制器对象（如 Job），用于可观测性和工具集成，不参与调度。

## 关键机制或特性
- **Pod 组（Pod Groups）**：每个组必须具有唯一的名称和一个调度策略（`basic` 或 `gang`）。
- **Gang 调度**：通过 `gang` 策略实现“全有或全无”调度，确保紧耦合工作负载的所有 Pod 能够同时调度，避免部分启动导致的死锁或资源浪费。
- **Pod 引用**：Pod 通过 `spec.workloadRef` 链接到 Workload 对象中的具体 Pod 组。

## 使用场景
- 大规模分布式训练（如 MPI、PyTorch、TensorFlow）需要 gang 调度。
- 批处理作业中多个 worker 必须同时启动才能协同工作。
- 为调度器提供显式的 Pod 分组信息，优化放置决策和集群可观测性。

## 最佳实践/注意事项
- 使用本特性前需确认集群启用了对应的特性门控和 API 组。
- `controllerRef` 仅用于工具和可观测性，调度器不会读取该字段。
- 若 Pod 引用了不存在的 Workload 或 Pod 组，Pod 将保持 Pending 状态。
- Gang 调度策略需同时启用 `GangScheduling` 特性门控。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/workload-api/
