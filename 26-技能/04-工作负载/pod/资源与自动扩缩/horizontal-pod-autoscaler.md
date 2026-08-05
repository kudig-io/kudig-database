---
title: Horizontal Pod Autoscaler
summary: Horizontal Pod Autoscaler（HPA）是 Kubernetes 中实现 Pod 水平自动扩缩容的核心控制器。它根据观测到的负载指标自动调整
  Deployment、StatefulSet 或 ReplicaSet 的副本数量，以匹配应用的实际流量需求，从而在性能与成本之间取得动态平衡。
category: concepts
tags:
- core-concept
- domain-02
- visibility/public
tier: core
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Horizontal Pod Autoscaler

Horizontal Pod Autoscaler（HPA）是 Kubernetes 中实现 Pod 水平自动扩缩容的核心控制器。它根据观测到的负载指标自动调整 Deployment、StatefulSet 或 ReplicaSet 的副本数量，以匹配应用的实际流量需求，从而在性能与成本之间取得动态平衡。

## 架构与数据流

HPA 的工作依赖以下组件协同：

1. **Metrics Server**：集群层面的资源指标聚合器，通过 kubelet 的 Summary API 收集 CPU、内存等核心指标。它是 HPA 获取 Resource Metrics 的唯一官方途径。
2. **HPA Controller**：kube-controller-manager 中的控制循环，定期（默认 15 秒）查询 Metrics Server 或 Custom Metrics API，获取目标工作负载的当前指标值。
3. **Deployment/ReplicaSet**：HPA 通过修改 `.spec.replicas` 字段驱动副本数量变化，再由底层控制器完成 Pod 创建或删除。

整个数据流为：`Pod → kubelet → Metrics Server → HPA Controller → Deployment → ReplicaSet → Pod`。

## 三种指标类型

- **Resource Metrics（资源指标）**：CPU 利用率、内存利用率。最常用，依赖 Metrics Server，仅支持 `type: Utilization`（百分比）或 `type: AverageValue`（绝对值）。要求 Pod 必须设置 `resources.requests`，否则无法计算利用率。
- **Custom Metrics（自定义指标）**：应用层指标，如 QPS、队列深度、活跃连接数。需要部署 Prometheus Adapter 或 Custom Metrics API Server，将 Prometheus 指标暴露为 Kubernetes API。
- **External Metrics（外部指标）**：集群外部系统的指标，如 Kafka Lag、云监控指标、SQS 队列长度。适用于事件驱动或云托管中间件场景。需要 External Metrics API 支持。

## 计算公式

HPA 的核心扩缩容公式为：

```
desiredReplicas = ceil[currentReplicas * (currentMetricValue / desiredMetricValue)]
```

例如，当前 4 个副本，CPU 平均利用率 80%，目标 50%，则 `desiredReplicas = ceil[4 * (80/50)] = ceil[6.4] = 7`。

当存在多条指标规则时，HPA 取各指标计算出的 `desiredReplicas` 中的**最大值**，确保所有指标都得到满足。若任一指标无法获取（如 Metrics Server 问题），HPA 将回退到上一次已知值，不会贸然缩容。

## 行为配置

Kubernetes 1.18+ 引入 `behavior` 字段，允许精细化控制扩缩容行为：

```yaml
behavior:
  scaleDown:
    stabilizationWindowSeconds: 300
    policies:
    - type: Percent
      value: 10
      periodSeconds: 60
  scaleUp:
    stabilizationWindowSeconds: 0
    policies:
    - type: Percent
      value: 100
      periodSeconds: 15
```

- **`scaleUp`**：定义扩容策略，如 `stabilizationWindowSeconds`（等待窗口，防止抖动）、`policies`（每次扩容的副本上限或百分比上限）。例如限制每分钟最多扩容 100% 或 4 个副本。
- **`scaleDown`**：定义缩容策略，默认 `stabilizationWindowSeconds` 为 300 秒，避免流量波动导致频繁缩容。生产环境建议保守配置缩容策略，防止流量突增时副本不足。

合理配置行为参数可防止因指标毛刺导致的副本剧烈震荡，提升系统稳定性。

## 远程顾问诊断要点

在远程顾问模式下，HPA 不扩容是最常被问到的问题。排查应遵循从指标源到控制器的顺序，逐层验证：

1. **Metrics Server 未安装或异常**：HPA 依赖 Metrics Server 获取指标。若 Metrics Server Pod 未运行或其 API 不可达，`kubectl top pod` 也会失败，HPA 无法获取当前值，自然无法扩容。指导用户检查 `kube-system` 命名空间下的 Metrics Server 状态及其 Service 是否正常。
2. **Target 未达阈值**：确认指标公式中的 `currentMetricValue` 是否真的超过了 `desiredMetricValue`。用户常因资源 request 设置过大导致利用率始终低于阈值。例如 CPU request 设为 4 核，实际使用 1 核，利用率仅 25%，即使目标 50% 也不会触发扩容。建议合理设置 request 与目标值的比例。
3. **Cooldown 窗口限制**：HPA 在扩容或缩容后有一段稳定期。若用户刚经历过一次扩容，新 Pod 尚未 Ready 导致平均指标回落，可能暂时无法再次扩容。建议查看 `kubectl describe hpa` 中的 `Conditions` 和 `Events` 字段获取时间线与原因。
4. **maxReplicas 已达上限**：检查 HPA 的 `maxReplicas` 是否已等于当前副本数，或 Deployment 本身受其他配额（ResourceQuota）限制无法继续创建 Pod。同时检查集群整体节点资源是否充足。
5. **指标类型配置错误**：若使用 Custom Metrics 或 External Metrics，确认 Prometheus Adapter 或对应指标暴露器正常运行，且 HPA 中 `metrics` 字段的 API 版本与指标名称拼写正确。

更多排查细节可参考 [[19-故障诊断/02-资源排障/09-hpa-vpa-troubleshooting.md|hpa-vpa-troubleshooting]] 与技能页面 [[19-故障诊断/02-资源排障/09-hpa-vpa-troubleshooting.md|k8s-hpa-vpa]]。

## 源码实现分析

### HPA Controller 调谐循环

```go
// kubernetes/pkg/controller/podautoscaler/horizontal.go
func (a *HorizontalController) reconcileKey(ctx context.Context, key string) error {
    // 1. 获取 HPA 对象
    hpa := a.hpaLister.Get(key)
    
    // 2. 获取当前指标值
    currentReplicas := a.getReplicas(hpa)  // Deployment.Status.Replicas
    metricsStatuses := a.computeReplicasForMetrics(hpa, currentReplicas)
    // 内部调用: metricsClient.GetResourceMetric("cpu", namespace, selector)
    // → Metrics Server API → kubelet Summary API
    
    // 3. 计算期望副本数（取所有指标的最大值）
    desiredReplicas := max(allMetricReplicas)
    // 公式: ceil(currentReplicas * currentMetric / targetMetric)
    
    // 4. 应用 behavior 约束（稳定窗口 + 速率限制）
    desiredReplicas = a.normalizeDesiredReplicas(hpa, currentReplicas, desiredReplicas)
    
    // 5. 更新 Deployment.Spec.Replicas
    a.updateReplicas(hpa, desiredReplicas)
}
```

### 指标获取链路

```
kubelet (cAdvisor) → Summary API (/stats/summary)
    │
    ▼
Metrics Server (聚合所有节点) → metrics.k8s.io API
    │
    ▼
HPA Controller (每 15s 查询一次)
    │
    ▼
计算 desiredReplicas → 更新 Deployment.Spec.Replicas
```

## 使用场景

### 场景一：基于 CPU 的标准 HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容等待 5 分钟
      policies:
      - type: Percent
        value: 10          # 每次最多缩 10%
        periodSeconds: 60
```

### 场景二：基于自定义指标（Prometheus Adapter）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: worker-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: queue-worker
  minReplicas: 2
  maxReplicas: 50
  metrics:
  - type: Pods
    pods:
      metric:
        name: rabbitmq_queue_messages_ready  # Prometheus 指标名
      target:
        type: AverageValue
        averageValue: "10"   # 每 Pod 平均队列深度 ≤ 10
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| HPA 可以没有 requests | 必须设置 CPU/内存 requests，否则无法计算利用率 |
| HPA 和 VPA 可同时用于同一指标 | 同一指标不能同时用 HPA+VPA，会冲突（可用不同指标） |
| 缩容是立即执行的 | 默认 300s 稳定窗口，防止流量波动导致频繁缩容 |
| HPA 能缩到 0 | HPA minReplicas 最小为 1，缩到 0 需 KEDA |
| 多指标取平均值 | 多指标取计算结果的“最大值”，确保所有指标都满足 |
| HPA 直接创建/删除 Pod | HPA 只修改 replicas 字段，由 Deployment/RS 控制器管理 Pod |

## 面试要点

1. **HPA 的扩缩容公式？** — `desiredReplicas = ceil[currentReplicas × (currentMetric / targetMetric)]`。多指标取最大值。若指标不可用，HPA 保持当前副本不变（安全回退）。

2. **HPA 与 Cluster Autoscaler 的关系？** — HPA 调整 Pod 副本数（水平扩展），CA 调整节点数（基础设施层）。HPA 扩容后若节点资源不足，Pod Pending 触发 CA 添加节点。两者协同工作但独立运行。

3. **如何避免 HPA 抱动（flapping）？** — 配置 behavior.scaleDown.stabilizationWindowSeconds（默认300s）；设置合理的扩容阈值（不要贴近实际负载）；使用 Percent 策略限制每次缩容幅度；避免指标源抨动（Prometheus 平滑窗口）。

4. **KEDA 与 HPA 的区别？** — HPA 最小副本为 1，KEDA 支持缩到 0（事件驱动）；KEDA 提供 60+ 外部触发器（Kafka/SQS/Cron）；KEDA 本质是生成 HPA 对象 + 管理 ScaledObject CRD。

## 相关概念

- [[autoscaling-strategies]] — Kubernetes 自动伸缩策略总览
- [[resource-management]] — Pod 资源管理机制
- [[deployment-controller-architecture]] — Deployment 控制器架构

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
