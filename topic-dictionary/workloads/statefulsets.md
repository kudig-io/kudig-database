# StatefulSets

## 概述
StatefulSet 是用于管理有状态应用的工作负载 API 对象。它管理一组基于相同容器规范运行的 Pod，并保证这些 Pod 的排序和唯一性。与 Deployment 不同，StatefulSet 为每个 Pod 维护一个粘性标识（sticky identity），即使 Pod 被重新调度，该标识也不会改变。

## 核心概念/原理
- **稳定网络标识**：每个 Pod 都有一个基于序号的唯一主机名，格式为 `$(statefulset-name)-$(ordinal)`。配合 Headless Service 可提供稳定的 DNS 名称。
- **稳定存储**：通过 `volumeClaimTemplates` 为每个 Pod 自动创建 PersistentVolumeClaim。Pod 重新调度后，原有的 PVC 会重新挂载到新 Pod。
- **有序部署与扩缩容**：默认 `OrderedReady` 策略下，Pod 按序号 0 到 N-1 依次创建；缩容时按 N-1 到 0 依次删除。每个前置 Pod 必须 Running 且 Ready 后，才会继续下一步。
- **Pod 序号**：
  - 默认从 0 开始。
  - 自 v1.31 起可通过 `spec.ordinals.start` 自定义起始序号。
  - 控制器会自动添加标签 `apps.kubernetes.io/pod-index`（值为序号）。

## 关键机制或特性
- **Pod 管理策略**：
  - `OrderedReady`（默认）：严格按顺序创建和删除。
  - `Parallel`：并行创建和终止所有 Pod，不等待前一个就绪。
- **更新策略**：
  - `RollingUpdate`（默认）：按逆序逐个删除并重建 Pod。支持 `partition` 进行灰度更新；支持 `maxUnavailable`（Beta，默认 1）控制同时不可用的 Pod 数。
  - `OnDelete`：不自动更新，需手动删除 Pod 触发重建。
- **版本控制与回滚**：使用 ControllerRevision 保存历史配置，支持 `kubectl rollout history/undo` 回滚到指定版本。可通过 `revisionHistoryLimit` 控制保留数量。
- **PVC 保留策略（v1.32 Stable）**：通过 `persistentVolumeClaimRetentionPolicy` 配置 `whenDeleted` 和 `whenScaled` 策略（`Delete` 或 `Retain`），决定在 StatefulSet 删除或缩容时是否自动删除对应的 PVC。默认行为为 `Retain`。
- **最小就绪时间（`minReadySeconds`）**：Pod 就绪后需持续 healthy 的最短时间，才被视为可用。

## 使用场景
- 需要稳定网络标识和持久存储的数据库（如 MySQL、PostgreSQL、MongoDB）。
- 分布式协调服务（如 ZooKeeper、etcd）。
- 消息队列（如 Kafka、RabbitMQ）。

## 最佳实践/注意事项
- 必须为 StatefulSet 创建对应的 Headless Service，以控制 Pod 的网络域。
- StatefulSet 名称必须是有效的 DNS Label。
- 使用 `OnDelete` 策略时要特别注意，需要手动删除 Pod 才能应用更新。
- 使用 `partition` 进行灰度发布时，确保序号大于等于 partition 的 Pod 才会更新。
- 强烈建议不要设置 `pod.spec.terminationGracePeriodSeconds` 为 0，以确保安全终止。
- 设置 PVC 保留策略前需确保 API Server 和 Controller Manager 启用了 `StatefulSetAutoDeletePVC` 特性门控。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/
