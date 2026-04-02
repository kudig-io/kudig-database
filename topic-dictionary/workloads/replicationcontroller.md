# ReplicationController

## 概述
ReplicationController 是一种遗留 API，用于确保指定数量的 Pod 副本始终处于运行状态。它已被 Deployment 和 ReplicaSet 取代，仅在维护旧系统或学习 Kubernetes 历史时可能遇到。

## 核心概念/原理
- **副本保障**：ReplicationController 持续监控与选择器匹配的 Pod 数量，过多则删除，过少则创建。
- **Pod 模板（`spec.template`）**：必填字段，`restartPolicy` 只能为 `Always`。
- **标签选择器（`spec.selector`）**：基于等式（equality-based）的选择器，管理所有匹配的 Pod，不论其创建者是谁。
- **Pod 替换**：当 Pod 因节点故障、维护或被删除而终止时，ReplicationController 会自动创建新的替代 Pod。

## 关键机制或特性
- **扩缩容**：通过修改 `spec.replicas` 即可手动扩缩容；也支持外部 autoscaler 修改。
- **滚动更新**：ReplicationController 本身不直接支持声明式滚动更新。推荐做法是先创建新的 ReplicationController，然后逐个缩放新旧控制器，最后删除旧控制器。
- **删除策略**：
  - `kubectl delete rc` 默认级联删除所有 Pod。
  - `--cascade=orphan` 可仅删除 ReplicationController 而保留 Pod，新的同选择器控制器可收养这些 Pod。
- **与 Service 配合**：多个 ReplicationController 可共享同一个 Service，实现金丝雀发布或多版本共存。

## 使用场景
- 维护历史遗留系统。
- 作为学习 Kubernetes 控制器原理的参考。
- **新系统应完全避免直接使用 ReplicationController**。

## 最佳实践/注意事项
- **强烈建议使用 Deployment 替代 ReplicationController**，因为 Deployment 提供声明式滚动更新、回滚和更丰富的生命周期管理。
- 若需要基于集合（set-based）的选择器，应使用 ReplicaSet。
- 避免创建标签与现有 ReplicationController 选择器重叠的裸 Pod，否则会被意外收养。
- 对于预期自行终止的任务，使用 Job；对于节点级服务，使用 DaemonSet。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/replicationcontroller/
