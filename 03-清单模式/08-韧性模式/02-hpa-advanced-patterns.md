---
title: HPA 高级配置模式
description: HorizontalPodAutoscaler 高级配置、自定义指标与行为策略
summary: HPA v2 高级配置，包括自定义指标、外部指标、行为策略（扩缩容速率）及与 VPA 协同
category: manifests-patterns
tags:
- k8s
- manifests
- reliability
- hpa
- autoscaling
- metrics
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- HPA 高级配置
- HPA 自定义指标
- HPA behavior 行为策略
trigger_keywords:
- hpa
- autoscaling
- custom-metrics
- behavior
- scale
prerequisites:
- k8s-deployment-basics
- metrics-server
authors:
- name: KUDIG Team
  role: contributor
---

# HPA 高级配置模式

## 1. HPA v2 完整结构

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 3
  maxReplicas: 50
  metrics:                       # 指标定义
    - ...
  behavior:                      # 扩缩容行为
    scaleUp:
      ...
    scaleDown:
      ...
```

## 2. 多指标组合

```yaml
metrics:
  # CPU 利用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  # 内存利用率
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  # 自定义指标（每 Pod QPS）
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "100"      # 每实例 100 QPS
  # 外部指标（如 SQS 队列深度）
  - type: External
    external:
      metric:
        name: sqs_queue_depth
        selector:
          matchLabels:
            queue: processing-queue
      target:
        type: AverageValue
        averageValue: "30"       # 每 30 条消息扩 1 个
```

> HPA 取所有指标中最大的需求副本数作为最终结果。

## 3. Behavior 行为策略

### 3.1 扩容策略（快速扩容）

```yaml
behavior:
  scaleUp:
    stabilizationWindowSeconds: 0    # 立即扩容（无稳定窗口）
    selectPolicy: Max                # 选最大扩容策略
    policies:
      - type: Percent
        value: 100                   # 可以翻倍
        periodSeconds: 30
      - type: Pods
        value: 5                     # 或最多加 5 个
        periodSeconds: 30
```

### 3.2 缩容策略（缓慢缩容）

```yaml
behavior:
  scaleDown:
    stabilizationWindowSeconds: 300  # 5 分钟稳定期
    selectPolicy: Min                # 选最小缩容策略（保守）
    policies:
      - type: Pods
        value: 1                     # 每次最多减 1 个
        periodSeconds: 120           # 每 2 分钟
      - type: Percent
        value: 10                    # 或最多减 10%
        periodSeconds: 120
```

## 4. 完整生产配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-api-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  minReplicas: 3
  maxReplicas: 100
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
          selector:
            matchLabels:
              app: web-api
        target:
          type: AverageValue
          averageValue: "200"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 30
      selectPolicy: Max
      policies:
        - type: Percent
          value: 50
          periodSeconds: 30
        - type: Pods
          value: 10
          periodSeconds: 30
    scaleDown:
      stabilizationWindowSeconds: 600  # 10 分钟
      selectPolicy: Min
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
```

## 5. 自定义指标（Prometheus Adapter）

```yaml
# Prometheus Adapter 规则
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-adapter
  namespace: monitoring
data:
  config.yaml: |
    rules:
      - seriesQuery: 'http_requests_total{kubernetes_namespace!="",kubernetes_pod_name!=""}'
        resources:
          overrides:
            kubernetes_namespace: {resource: "namespace"}
            kubernetes_pod_name: {resource: "pod"}
        name:
          matches: "^(.*)_total"
          as: "${1}_per_second"
        metricsQuery: 'rate(<<.Series>>{<<.LabelMatchers>>}[2m])'
```

## 6. HPA + VPA 协同

```yaml
# 注意：HPA 和 VPA 不能同时管理同一资源维度
# 正确做法：HPA 管 CPU/自定义指标，VPA 只管内存
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: Auto
  resourcePolicy:
    containerPolicies:
      - name: app
        controlledResources: ["memory"]  # VPA 只管内存
        minAllowed:
          memory: 128Mi
        maxAllowed:
          memory: 2Gi
```

## 7. HPA 与 Cluster Autoscaler/Karpenter

```
流量增加
  ↓
HPA: CPU 超阈值 → 需要 10 个 Pod
  ↓
调度器: 节点资源不足 → Pod Pending
  ↓
Cluster Autoscaler/Karpenter: 检测 Pending Pod → 自动扩容节点
  ↓
新节点就绪 → Pending Pod 被调度
  ↓
Pod 启动 → 负载分担
```

## 8. 预缩容配置（预热）

```yaml
# Cron 预扩容（结合 keda-cron 或 Scheduled HPA）
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: scheduled-scale-up
spec:
  scaleTargetRef:
    name: web-api
  minReplicaCount: 3
  maxReplicaCount: 50
  triggers:
    - type: cron
      metadata:
        timezone: Asia/Shanghai
        start: "0 8 * * 1-5"       # 工作日 8 点
        end: "0 20 * * 1-5"         # 工作日 20 点
        desiredReplicas: "20"       # 预扩容到 20
```

## 9. 调试与验证

```bash
# 🟢 低风险：HPA 调试
# 查看 HPA 状态和推荐
kubectl describe hpa web-api-hpa -n production

# 查看 HPA 计算过程
kubectl get hpa web-api-hpa -n production -o yaml

# 检查指标是否可用
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta1/namespaces/production/pods/*/http_requests_per_second"

# 查看 metrics-server 是否正常
kubectl top pods -n production
```

## 10. 生产实践

| 实践 | 说明 |
|------|------|
| 扩容快缩容慢 | `scaleUp.stabilizationWindowSeconds: 0`，`scaleDown: 300+` |
| 设置合理的 minReplicas | 保证基线可用性 |
| 使用自定义指标优于 CPU | 更贴近业务 |
| 监控 HPA 决策 | 确认指标正确 |
| 与 Cluster Autoscaler 配合 | Pod Pending 触发节点扩容 |
| 避免 HPA 与手动 scale 冲突 | 手动 scale 会被 HPA 覆盖 |
| 注意冷启动时间 | 新 Pod 需要预热 |

## Related

- [[03-清单模式/08-韧性模式/03-vpa-patterns|VPA 集成模式]]
- [[03-清单模式/08-韧性模式/04-karpenter-nodepool-patterns|Karpenter NodePool]]
- [[03-清单模式/01-YAML参考/27-hpa-autoscaling-v2|HPA 参考文档]]

## See Also

- [HPA v2 文档](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)
- [Prometheus Adapter](https://github.com/kubernetes-sigs/prometheus-adapter)

<!-- risk-assessed -->
