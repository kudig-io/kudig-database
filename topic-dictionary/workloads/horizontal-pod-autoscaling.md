# Horizontal Pod Autoscaling

## 概述
HorizontalPodAutoscaler（HPA）是 Kubernetes 的 API 资源和控制器，可根据观察到的指标（如 CPU 利用率、内存利用率或自定义指标）自动调整工作负载（Deployment、StatefulSet 等）的副本数量。

## 核心概念/原理
- **控制循环**：HPA 控制器在 kube-controller-manager 中以固定周期运行（默认 15 秒），查询指标并调整目标副本数。
- **缩放目标**：通过 `scaleTargetRef` 指向支持 `scale` 子资源的工作负载（如 Deployment、StatefulSet、ReplicaSet）。
- **副本数计算**：
  ```
  desiredReplicas = ceil(currentReplicas * currentMetricValue / desiredMetricValue)
  ```
  当比值接近 1.0 时（默认容差 10%），控制器跳过缩放动作。
- **缺失指标与未就绪 Pod 处理**：
  - 缺失指标的 Pod 在缩容时按 100% 利用率假设，在扩容时按 0% 假设，以保守方式 dampen 缩放幅度。
  - 未就绪 Pod 的 CPU  metrics 在初始就绪延迟（默认 30 秒）和 CPU 初始化期（默认 5 分钟）内可能被忽略。

## 关键机制或特性
- **指标类型（autoscaling/v2）**：
  - **Resource metrics**：基于 Pod 级别的 CPU 或内存利用率/原始值。
  - **Container resource metrics**（v1.30 Stable）：基于特定容器的资源使用进行缩放。
  - **Custom metrics**：通过 `custom.metrics.k8s.io` 获取的自定义指标。
  - **External metrics**：通过 `external.metrics.k8s.io` 获取的外部指标。
- **多指标支持**：可配置多个指标，HPA 会计算每个指标对应的期望副本数，最终取最大值。
- **行为配置（`behavior`）**：
  - `scaleUp` / `scaleDown`：分别配置扩容和缩容行为。
  - `policies`：定义缩放速率（按 Pods 数或百分比）。
  - `stabilizationWindowSeconds`：缩容稳定窗口，默认 300 秒，用于平滑副本波动。
  - `selectPolicy`：`Max`（默认，允许最大变化）、`Min`（最小变化）、`Disabled`（禁用该方向缩放）。
  - `tolerance`（v1.35 Beta）：指标波动容差，默认 10%；例如目标 100MiB、容差 5%，则仅在超过 105MiB 时才扩容。
- **Pod 就绪与启动**：
  - `--horizontal-pod-autoscaler-initial-readiness-delay`（默认 30s）
  - `--horizontal-pod-autoscaler-cpu-initialization-period`（默认 5m）

## 使用场景
- 流量波动明显的无状态 Web 服务和 API。
- 需要基于队列长度、请求延迟等自定义指标自动扩容的场景。
- 在滚动更新期间保持应用可用性并自动调整容量。

## 最佳实践/注意事项
- 使用 `autoscaling/v2` API 以利用多指标、行为配置和容器级资源指标。
- 确保 Metrics Server（resource metrics）或相应的 custom/external metrics adapter 已部署。
- 使用 HPA 时，建议从目标工作负载的 manifest 中移除 `spec.replicas`，避免 `kubectl apply` 引起副本数抖动（thrashing）。
- 对于启动期 CPU 突增的应用，配置合适的 `startupProbe` 或 `readinessProbe`，并确保 `cpu-initialization-period` 覆盖启动时长。
- 滚动更新期间，Deployment 控制器与 HPA 协同管理 ReplicaSet 副本数；StatefulSet 则由 StatefulSet 控制器直接处理。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/autoscaling/horizontal-pod-autoscale/
