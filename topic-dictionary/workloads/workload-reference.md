# Workload Reference

## 概述
Workload Reference 是 Kubernetes v1.35 引入的 Alpha 特性（默认禁用，需启用 `GenericWorkload` 特性门控）。它允许将 Pod 链接到一个 Workload 对象，使调度器能够按组进行协同调度决策，而不是将 Pod 视为独立个体。

## 核心概念/原理
- **Workload 引用**：在 Pod 的 `spec.workloadRef` 字段中指定同一命名空间下的 Workload 对象名称和 Pod 组名称。
- **Pod 组副本（podGroupReplicaKey）**：通过 `podGroupReplicaKey` 可将单个 Pod 组复制为多个独立的调度单元。例如，设置不同的 replica key 可创建多个逻辑子组。
- **行为**：
  - 若引用的组使用 `basic` 策略，workloadRef 主要起分组标签作用。
  - 若引用 `gang` 策略（需启用 `GangScheduling`），Pod 将进入 gang 调度生命周期，等待组内其他 Pod 就绪后一起绑定到节点。
- **缺失引用处理**：若 Pod 引用的 Workload 或 Pod 组不存在，Pod 将保持 Pending，不会被调度。

## 关键机制或特性
- **协同调度**：适用于紧耦合应用（如分布式训练 Job），需要一组 Pod 同时启动才能正常工作。
- **调度器验证**：调度器在做出放置决策前会验证 `workloadRef` 的有效性。

## 使用场景
- 大规模机器学习训练任务（如 MPI、PyTorch），需要所有 worker 同时运行。
- 需要 gang 调度的批处理作业，避免部分启动导致死锁或资源浪费。
- 将 Pod 按应用组进行逻辑归类，便于可观测性和管理。

## 最佳实践/注意事项
- 使用该特性前需确保集群启用了 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组。
- 使用 `gang` 策略时，还需启用 `GangScheduling` 特性门控。
- 确保 Workload 对象和 Pod 组在 Pod 被调度前已存在，否则 Pod 将无限期 Pending。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/workload-reference/
