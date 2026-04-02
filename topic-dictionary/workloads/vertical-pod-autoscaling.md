# Vertical Pod Autoscaling

## 概述
VerticalPodAutoscaler（VPA）自动调整工作负载（如 Deployment、StatefulSet）中 Pod 的资源请求（requests）和限制（limits），以匹配实际资源使用情况。这种垂直缩放也称为 rightsizing 或 autopilot。

## 核心概念/原理
- **VPA 组成**：
  - **Recommender**：分析 Pod 的历史和实时资源使用，生成推荐值（target、lower bound、upper bound）。
  - **Updater**：监控推荐值与当前 Pod 资源的差异，必要时通过驱逐 Pod 或原地更新来应用新资源。
  - **Admission Controller**：以 mutating webhook 形式拦截 Pod 创建请求，将推荐资源注入到新 Pod 中。
- **指标来源**：VPA 需要 Metrics Server（`metrics.k8s.io`）提供资源使用数据。
- **API 版本**：稳定 API 为 `autoscaling.k8s.io/v1`，以 CRD 形式提供，需单独安装。

## 关键机制或特性
- **更新模式（`updateMode`）**：
  - `Off`：仅生成推荐，不自动应用。
  - `Initial`：仅在 Pod 首次创建时应用推荐，不更新运行中的 Pod。
  - `Recreate`：当推荐与当前资源差异超过阈值时，驱逐 Pod 并由控制器重建以应用新资源。
  - `InPlaceOrRecreate`：优先尝试原地更新资源；若不支持则回退到驱逐重建（需集群支持原地 resize）。
  - `Auto`（已弃用，VPA 1.4.0+）：别名等同于 `Recreate`。
- **资源策略（`resourcePolicy`）**：
  - `minAllowed` / `maxAllowed`：为推荐值设置上下限。
  - `controlledResources`：指定 VPA 管理的资源类型（`cpu`、`memory`）。
  - `controlledValues`：
    - `RequestsAndLimits`（默认）：同时调整 request 和 limit，limit 按原始 request-to-limit 比例缩放。
    - `RequestsOnly`：仅调整 request，保持 limit 不变。
- **LimitRange 兼容**：Admission Controller 和 Updater 会确保推荐值符合命名空间中 LimitRange 的约束。
- **PDB 尊重**：Updater 在驱逐 Pod 时会遵守 PodDisruptionBudget，尽量减少服务影响。

## 使用场景
- 难以准确预估资源需求的应用，希望自动优化资源配置。
- 需要避免资源浪费（过度分配）或应用因资源不足而 OOM/Crash 的场景。
- 与 Cluster Autoscaler 配合，通过更准确的资源请求改善节点利用率。

## 最佳实践/注意事项
- 安装 VPA 前确保 Metrics Server 已正常运行。
- 若对同一工作负载同时使用 HPA 和 VPA，需谨慎配置，避免两者冲突。常见做法是：HPA 基于自定义指标缩放，VPA 仅调整资源请求（`RequestsOnly` 模式）。
- 使用 `Recreate` 模式时，注意 Pod 重建会带来的短暂中断；对中断敏感的服务可评估 `InPlaceOrRecreate` 或 `Initial` 模式。
- 使用 `minAllowed` 和 `maxAllowed` 限制推荐范围，防止极端推荐导致应用异常。
- VPA 不适用于 DaemonSet（通常使用 Cluster Proportional Vertical Autoscaler 替代）。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/autoscaling/vertical-pod-autoscale/
