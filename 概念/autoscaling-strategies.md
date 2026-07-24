---
title: Autoscaling Strategies
description: Autoscaling Strategies — Kubernetes 生产运维知识库
summary: Autoscaling Strategies — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- autoscaling
- hpa
- vpa
- cluster-autoscaler
- karpenter
- prometheus
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Autoscaling Strategies 是什么
- 如何 Autoscaling Strategies
trigger_keywords:
- Autoscaling
- Strategies
prerequisites:
- kubectl-basics
- prometheus-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Autoscaling Strategies

## Horizontal Pod Autoscaler (HPA)

HPA scales the number of Pod replicas based on observed metrics:

| Metric Type | Source | Example |
|-------------|--------|---------|
| **CPU/Memory** | metrics-server | Scale when avg CPU > 70% |
| **Custom metrics** | Prometheus adapter | Scale on requests per second |
| **External metrics** | Cloud provider | Scale on queue depth |

**Key behaviors**:
- Polling interval: ~15 seconds (default)
- Stabilization window: Prevents flapping (scale-up: 0s, scale-down: 5min default)
- Works with Deployment, [[StatefulSet|StatefulSet]], [[ReplicaSet|ReplicaSet]]

## Vertical Pod Autoscaler (VPA)

VPA adjusts resource requests/limits based on actual usage:

| Mode | Behavior | Production Safe |
|------|----------|-----------------|
| **Off** | Only recommends | Yes |
| **Initial** | Sets on Pod creation | Yes |
| **Auto** | Updates existing [[Pods|Pods]] (recreates) | Yes, with caution |
| **Recreate** | Same as Auto | Yes, with caution |

**Warning**: VPA and HPA on the same resource (CPU/memory) will conflict -- use VPA for right-sizing and HPA for replica scaling.

## Cluster Autoscaler / Karpenter

- **Cluster Autoscaler**: Traditional node scaler; adds/removes nodes based on unschedulable Pods
- **Karpenter**: Next-generation, faster node provisioning; supports spot instances, flexible instance selection, and TTL-based cleanup

## Scaling Strategy Recommendations

- Use **HPA** for demand-driven replica scaling (stateless services)
- Use **VPA** for right-sizing resource requests (all workloads)
- Use **Cluster Autoscaler** or **Karpenter** for node pool scaling
- Combine HPA + VPA (on different metrics) + Cluster Autoscaler for full automation

## 实践示例

### HPA 配置 (CPU + 自定义指标)

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
  maxReplicas: 50
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 30
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
```

### VPA 配置 (推荐模式)

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  updatePolicy:
    updateMode: "Off"  # 仅推荐，不自动修改
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: 100m
        memory: 128Mi
      maxAllowed:
        cpu: "4"
        memory: 8Gi
```

## 源码实现分析

### HPA Controller 扩缩容决策

```go
// k8s.io/kubernetes/pkg/controller/podautoscaler/horizontal.go
// HPA 核心计算逻辑
func (a *HorizontalController) computeReplicasForMetrics(hpa *autoscalingv2.HorizontalPodAutoscaler, scale *autoscalingv1.Scale, metrics []metricStatus) (int32, error) {
    // 1. 获取当前指标（从 metrics-server / external metrics adapter）
    currentReplicas := scale.Status.Replicas
    
    // 2. 计算期望副本数
    // desiredReplicas = ceil(currentReplicas * currentMetric / targetMetric)
    // 例: 当前 3 副本，CPU 80%，目标 50% → ceil(3 * 80/50) = 5
    desiredReplicas := int32(math.Ceil(
        float64(currentReplicas) * currentMetricValue / targetMetricValue,
    ))
    
    // 3. 应用 stabilizationWindowSeconds（默认 300s）
    // 防止指标波动导致频繁扩缩
    desiredReplicas = a.normalizeDesiredReplicas(hpa, currentReplicas, desiredReplicas)
    
    // 4. 限制在 [minReplicas, maxReplicas] 范围内
    return clamp(desiredReplicas, hpa.Spec.MinReplicas, hpa.Spec.MaxReplicas), nil
}
```

```
┌─────────────────────────────────────────────────────────┐
│     自动扩缩容策略对比                              │
├─────────────────────────────────────────────────────────┤
│  HPA: Pod 副本数 ← CPU/Memory/自定义指标          │
│    └─ 适合: 无状态服务、可水平扩展              │
│                                                         │
│  VPA: Pod 资源 requests/limits ← 历史使用量       │
│    └─ 适合: 有状态服务、单实例优化              │
│                                                         │
│  KEDA: Pod 副本数 ← 外部事件源 (Kafka/Queue)     │
│    └─ 适合: 事件驱动、缩容到 0                  │
│                                                         │
│  Cluster Autoscaler: 节点数 ← Pending Pod         │
│    └─ 适合: 节点资源不足时自动扩容              │
└─────────────────────────────────────────────────────────┘
```

### 生产配置：HPA + KEDA

```yaml
# HPA 基于自定义指标
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: api-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  minReplicas: 3
  maxReplicas: 50
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容冷却 5min
    scaleUp:
      stabilizationWindowSeconds: 60   # 扩容冷却 1min
  metrics:
  - type: Resource
    resource:
      name: cpu
      target: {type: Utilization, averageUtilization: 70}
  - type: Pods
    pods:
      metric: {name: http_requests_per_second}
      target: {type: AverageValue, averageValue: "1000"}
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| HPA 和 VPA 可以同时用于 CPU | 会冲突，HPA 用 CPU，VPA 用内存或自定义指标 |
| maxReplicas 越大越好 | 需考虑节点资源和成本 |
| VPA Auto 模式无风险 | 会重建 Pod，可能导致短暂中断 |
| Cluster Autoscaler 立即扩容 | 新节点启动需 1-3 分钟 |
| 缩容也是立即的 | 缩容有稳定窗口，防止抨动 |

## 面试要点

1. **HPA 的扩缩容决策是如何计算的？**
   - desiredReplicas = ceil(currentReplicas × currentMetric / targetMetric)
   - 稳定窗口防止抨动
   - 支持多指标取最大值

2. **VPA 的三种模式有什么区别？**
   - Off: 仅推荐，不修改
   - Initial: 仅创建时设置
   - Auto/Recreate: 自动重建 Pod 应用新资源

3. **Karpenter 相比 Cluster Autoscaler 的优势？**
   - 更快: 直接调用云 API，不依赖 ASG
   - 更灵活: 自动选择最优实例类型
   - 支持 Spot 实例和 TTL 清理

## Related

- [[实体/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[概念/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[概念/resource-management.md|Resource Management]]
- [[技能/工作负载/pod/运维操作/configure-health-probes.md|Configure Health Probes]]
- [[概念/scheduling-algorithm.md|Scheduling Algorithm]]


<!-- risk-assessed -->
