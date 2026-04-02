# Autoscaling Workloads

## 概述
自动扩缩容（Autoscaling）允许工作负载根据资源需求自动调整规模，使集群能够更弹性和高效地响应变化。Kubernetes 支持水平扩缩容（增加/减少副本数）和垂直扩缩容（调整单个 Pod 的资源）。

## 核心概念/原理
- **水平扩缩容（Horizontal Scaling）**：通过增加或减少 Pod 副本数来应对负载变化。
- **垂直扩缩容（Vertical Scaling）**：通过调整现有 Pod 的 CPU/内存 request/limit 来应对资源需求变化。
- **手动扩缩容**：
  - 水平：`kubectl scale` 或修改 `spec.replicas`。
  - 垂直：通过 patch 修改 Pod 或工作负载的资源定义，或使用原地 resize 功能。
- **自动扩缩容**：
  - **HPA（HorizontalPodAutoscaler）**：根据 CPU、内存或自定义指标自动调整副本数。
  - **VPA（VerticalPodAutoscaler）**：根据历史资源使用情况自动调整 Pod 的资源请求和限制。
  - **Cluster Proportional Autoscaler**：根据集群节点数/核心数自动水平扩缩容。
  - **Cluster Proportional Vertical Autoscaler**：根据集群规模自动垂直调整资源请求（Beta）。
  - **KEDA（Kubernetes Event Driven Autoscaler）**：基于事件（如队列消息数）驱动扩缩容。
  - **定时扩缩容**：可通过 KEDA 的 `Cron` scaler 按时间表扩缩容。

## 关键机制或特性
- **HPA**：Kubernetes 核心 API 资源和控制器，周期（默认 15 秒）根据指标调整目标副本数。
- **VPA**：以 CRD 形式提供，需单独安装。包含三个组件：Recommender（分析并生成推荐）、Updater（驱逐 Pod 或原地更新资源）、Admission Controller（在 Pod 创建时注入推荐资源）。
- **原地垂直扩缩容（In-place Pod Vertical Scaling，v1.35 Stable）**：允许在不重新创建 Pod 的情况下调整 CPU 和内存资源；VPA 与原地扩缩容的集成仍在发展中。
- **Metrics Server**：HPA 和 VPA 通常需要 Metrics Server 作为指标来源。

## 使用场景
- 流量波动大的 Web 应用和 API 服务，使用 HPA 自动扩容。
- 资源使用难以预估的应用，使用 VPA 自动 rightsizing。
- 系统级服务（如 DNS）需要根据集群规模自动调整，使用 Cluster Proportional Autoscaler。
- 基于消息队列的批处理任务，使用 KEDA 根据队列深度自动扩容。
- 需要在非高峰时段降本的场景，使用 KEDA Cron scaler 定时缩容。

## 最佳实践/注意事项
- 使用 HPA 时，建议从 Deployment/StatefulSet 清单中移除 `spec.replicas`，避免与声明式应用冲突。
- 部署 VPA 前需确认 Metrics Server 已安装并正常工作。
- HPA 和 VPA 同时作用于同一资源时需谨慎，可能出现冲突；通常建议对同一工作负载不同时启用两者的自动模式。
- 若工作负载级别的扩缩容仍无法满足需求，可进一步考虑节点自动扩缩容（Cluster Autoscaler）。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/autoscaling/
