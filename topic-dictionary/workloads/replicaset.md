# ReplicaSet

## 概述
ReplicaSet 的作用是维护一组稳定运行的 Pod 副本。它通常不直接使用，而是由 Deployment 自动管理，作为 Deployment 实现 Pod 创建、更新和扩缩容的底层机制。

## 核心概念/原理
- **核心字段**：
  - `spec.replicas`：目标副本数，默认 1。
  - `spec.selector`：标签选择器，用于识别和获取受管理的 Pod。必须与 `spec.template.metadata.labels` 匹配，创建后不可变。
  - `spec.template`：Pod 模板，`restartPolicy` 只能为 `Always`。
- **Pod 获取机制**：ReplicaSet 不仅管理自己创建的 Pod，也会立即获取与其选择器匹配且无控制器 OwnerReference（或 OwnerReference 非控制器）的裸 Pod。
- **Pod 替换**：当受管 Pod 被删除或终止时（如节点故障、维护），ReplicaSet 会自动创建替代 Pod。

## 关键机制或特性
- **扩缩容**：通过修改 `spec.replicas` 即可手动扩缩容；也支持作为 HPA 的缩放目标。
- **删除策略**：
  - 默认 `kubectl delete rs` 会级联删除所有 Pod。
  - 使用 `--cascade=orphan` 可仅删除 ReplicaSet 而保留 Pod；后续创建同名选择器的 ReplicaSet 可收养这些 Pod。
- **缩容算法**：缩容时按以下优先级选择要删除的 Pod：
  1. 未调度或 Pending 的 Pod
  2. 节点上该控制器 Pod 密度较高的
  3. 创建时间较新的
  4. `controller.kubernetes.io/pod-deletion-cost` 注解值较低的（Beta，默认启用）
  5. 随机选择
- **终止副本追踪（Beta）**：`DeploymentReplicaSetTerminatingReplicas` 启用后，可通过 `.status.terminatingReplicas` 查看终止中副本数。

## 使用场景
- 作为 Deployment 的底层实现，绝大多数情况下应通过 Deployment 间接使用。
- 仅在需要自定义更新编排或根本不需要更新时，才考虑直接使用 ReplicaSet。

## 最佳实践/注意事项
- **推荐做法**：日常应用管理应使用 Deployment，而非直接操作 ReplicaSet。
- 避免创建标签与现有 ReplicaSet 选择器重叠的裸 Pod，否则会被意外收养并可能导致终止。
- ReplicaSet 本身不支持滚动更新；如需受控更新，请使用 Deployment。
- 可利用 `pod-deletion-cost` 注解影响缩容时优先保留的 Pod。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/replicaset/
