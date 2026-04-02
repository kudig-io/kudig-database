# Deployments

## 概述
Deployment 为 Pod 和 ReplicaSet 提供声明式更新能力。用户描述期望状态，Deployment 控制器以受控速率将实际状态变更为期望状态。它是 Kubernetes 中管理无状态应用最常用的工作负载资源。

## 核心概念/原理
- **Pod 模板（`.spec.template`）**：定义 Pod 的规格，必须包含与应用选择器匹配的标签；`restartPolicy` 只能为 `Always`。
- **选择器（`.spec.selector`）**：标签选择器，用于识别 Deployment 管理的 Pod。创建后不可变。
- **副本数（`.spec.replicas`）**：期望运行的 Pod 数量，默认为 1。若由 HPA 管理，应避免在清单中硬编码该字段。
- **更新策略（`.spec.strategy`）**：
  - `RollingUpdate`（默认）：逐步创建新 Pod、删除旧 Pod。可配置 `maxSurge`（最大可超出副本数）和 `maxUnavailable`（最大不可用副本数），默认均为 25%。
  - `Recreate`：先删除所有旧 Pod，再创建新 Pod。
- **进度截止时间（`.spec.progressDeadlineSeconds`）**：默认 600 秒。若在此时间内未推进完成，Deployment 状态会标记为 `ProgressDeadlineExceeded`。
- **最小就绪时间（`.spec.minReadySeconds`）**：新 Pod 就绪后需持续 healthy 的最短时间，才被视为可用。
- **修订历史限制（`.spec.revisionHistoryLimit`）**：保留的旧 ReplicaSet 数量，默认 10，用于回滚。

## 关键机制或特性
- **版本管理**：每次修改 Pod 模板都会创建一个新的 ReplicaSet 作为修订版本。旧版本保留以便回滚。
- **回滚**：支持 `kubectl rollout undo` 回滚到上一版本或指定版本。
- **暂停/恢复**：`kubectl rollout pause` 可暂停滚动更新，允许累积多个修改后一次性生效。
- **比例缩放（Proportional Scaling）**：在滚动更新过程中收到扩缩容请求时，控制器会按现有活跃 ReplicaSet 的比例分配新增/减少的副本。
- **终止副本追踪（Beta）**：`DeploymentReplicaSetTerminatingReplicas` 特性门控启用后，可通过 `.status.terminatingReplicas` 查看处于终止状态的副本数。

## 使用场景
- 无状态 Web 应用和 API 服务的部署与更新。
- 需要零停机滚动发布和快速回滚能力的场景。
- 配合 HPA 实现自动水平扩缩容。

## 最佳实践/注意事项
- 确保选择器与 Pod 模板的标签匹配，且不要与其他控制器重叠。
- 若使用 HPA，建议从 manifest 中移除 `spec.replicas`，避免 `kubectl apply` 与 HPA 发生冲突。
- 为需要长时间预热的服务设置合理的 `minReadySeconds` 和 readiness probe。
- 设置合适的 `progressDeadlineSeconds` 以便及时发现卡住的发布。
- 注意 `maxSurge` 和 `maxUnavailable` 的配置对资源消耗和可用性的影响。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/deployment/
