---
title: Kubernetes Autoscaling Deep Dive — HPA/VPA/KEDA/Cluster Autoscaler
description: K8s 自动缩放深度实践 — HPA 自定义指标、VPA 推荐、KEDA 事件驱动、Cluster Autoscaler、缩放策略设计
summary: 全方位自动缩放体系设计，涵盖工作负载缩放与集群缩放的协调策略
category: practice
tags:
- autoscaling
- hpa
- vpa
- keda
- cluster-autoscaler
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: cluster
---
# Kubernetes 自动缩放深度实践

> 从工作负载到集群的全方位弹性伸缩体系。

## 缩放层次全景

| 层次 | 工具 | 缩放对象 | 触发条件 |
|------|------|----------|----------|
| 工作负载（水平） | HPA | Pod 副本数 | CPU/内存/自定义指标 |
| 工作负载（垂直） | VPA | Pod 资源请求 | 历史使用率 |
| 事件驱动 | KEDA | Pod 副本数 | 队列长度/外部事件 |
| 集群节点 | Cluster Autoscaler | 节点数 | Pending Pod |
| 集群节点（Serverless） | Karpenter | 节点数/类型 | Pending Pod + 约束 |

## HPA 高级配置

### 自定义指标 HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: api-server-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  minReplicas: 3
  maxReplicas: 50
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 30
      policies:
        - type: Percent
          value: 100        # 每次最多翻倍
          periodSeconds: 60
        - type: Pods
          value: 10         # 或每次最多加 10 个
          periodSeconds: 60
      selectPolicy: Max
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容等待 5 分钟
      policies:
        - type: Percent
          value: 10         # 每次最多缩 10%
          periodSeconds: 60
      selectPolicy: Min
  metrics:
    # CPU 利用率
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
    # 自定义指标：每秒请求数
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
        target:
          type: AverageValue
          averageValue: "1000"
    # 外部指标：消息队列深度
    - type: External
      external:
        metric:
          name: kafka_consumer_lag
          selector:
            matchLabels:
              topic: orders
              group: api-consumer
        target:
          type: AverageValue
          averageValue: "50"
```

### Prometheus Adapter 配置

```yaml
# 将 Prometheus 指标暴露为 K8s 自定义指标
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-adapter
data:
  config.yaml: |
    rules:
      - seriesQuery: 'http_requests_total{namespace!="",pod!=""}'
        resources:
          overrides:
            namespace: {resource: "namespace"}
            pod: {resource: "pod"}
        name:
          matches: "^(.*)_total"
          as: "${1}_per_second"
        metricsQuery: 'sum(rate(<<.Series>>{<<.LabelMatchers>>}[2m])) by (<<.GroupBy>>)'
      - seriesQuery: 'kafka_consumergroup_lag'
        resources:
          overrides:
            namespace: {resource: "namespace"}
        name:
          as: "kafka_consumer_lag"
        metricsQuery: 'sum(<<.Series>>{<<.LabelMatchers>>}) by (<<.GroupBy>>)'
```

## VPA 垂直缩放

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: api-server-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  updatePolicy:
    updateMode: "Auto"  # Off/Initial/Auto
    minReplicas: 2
  resourcePolicy:
    containerPolicies:
      - containerName: api
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: "8"
          memory: 16Gi
        controlledResources: ["cpu", "memory"]
        controlledValues: RequestsAndLimits
```

> **注意**：HPA 与 VPA 不应同时基于 CPU/内存缩放（冲突）。推荐 HPA 管水平 + VPA 仅做推荐（Off 模式）。

## KEDA 事件驱动缩放

```yaml
# Kafka 消费者缩放
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: order-consumer-scaler
  namespace: production
spec:
  scaleTargetRef:
    name: order-consumer
  pollingInterval: 15
  cooldownPeriod: 300
  minReplicaCount: 2
  maxReplicaCount: 100
  triggers:
    - type: kafka
      metadata:
        bootstrapServers: kafka:9092
        consumerGroup: order-processor
        topic: orders
        lagThreshold: "100"
        offsetResetPolicy: latest
---
# 基于 Prometheus 查询缩放
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: api-scaler
spec:
  scaleTargetRef:
    name: api-server
  minReplicaCount: 3
  maxReplicaCount: 30
  triggers:
    - type: prometheus
      metadata:
        serverAddress: http://prometheus:9090
        metricName: active_connections
        query: |
          sum(active_connections{deployment="api-server"})
        threshold: "500"
---
# Cron 定时缩放（工作时间扩容）
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: business-hours-scaler
spec:
  scaleTargetRef:
    name: web-frontend
  minReplicaCount: 2
  maxReplicaCount: 20
  triggers:
    - type: cron
      metadata:
        timezone: Asia/Shanghai
        start: "0 8 * * 1-5"
        end: "0 20 * * 1-5"
        desiredReplicas: "10"
```

## Karpenter（下一代节点缩放）

```yaml
# NodePool 定义
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: default
spec:
  template:
    spec:
      requirements:
        - key: kubernetes.io/arch
          operator: In
          values: ["amd64", "arm64"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand", "spot"]
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["m5.large", "m5.xlarge", "m6g.large", "m6g.xlarge"]
      nodeClassRef:
        name: default
  limits:
    cpu: "1000"
    memory: 2000Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h  # 30 天节点轮换
---
# EC2NodeClass
apiVersion: karpenter.k8s.aws/v1beta1
kind: EC2NodeClass
metadata:
  name: default
spec:
  amiFamily: AL2
  role: KarpenterNodeRole
  subnetSelectorTerms:
    - tags:
        karpenter.sh/discovery: my-cluster
  securityGroupSelectorTerms:
    - tags:
        karpenter.sh/discovery: my-cluster
  tags:
    Environment: production
    ManagedBy: karpenter
```

## 缩放策略设计

### 决策矩阵

| 工作负载类型 | 推荐方案 | 理由 |
|-------------|----------|------|
| Web API（稳定流量） | HPA (CPU) | 简单有效 |
| Web API（突发流量） | HPA + KEDA (Prometheus) | 快速响应 |
| 消息消费者 | KEDA (Kafka/RabbitMQ) | 基于队列深度 |
| 批处理任务 | KEDA (Cron) + Job | 定时执行 |
| 内存敏感型 | VPA (推荐) + HPA (自定义) | 避免 OOM |
| GPU 工作负载 | Karpenter + 节点池 | 昂贵资源精确调度 |

### 反模式

| 反模式 | 问题 | 解决 |
|--------|------|------|
| HPA + VPA 同时基于 CPU | 缩放冲突/震荡 | VPA 设为 Off 或基于不同指标 |
| 缩容过快 | 流量恢复时冷启动 | 增加 stabilizationWindow |
| 无 minReplicas | 缩到 0 后恢复慢 | 设置合理最小值 |
| 仅基于 CPU | 忽略实际业务负载 | 添加自定义指标 |
| 无 PDB | 缩容/升级时全部下线 | 配置 PodDisruptionBudget |

## 监控缩放行为

```promql
# HPA 当前/期望副本
kube_horizontalpodautoscaler_status_current_replicas
kube_horizontalpodautoscaler_spec_max_replicas

# 缩放事件
kubernetes_events{reason="SuccessfulRescale"}

# KEDA 缩放器活跃状态
keda_scaler_active

# Cluster Autoscaler 节点变化
cluster_autoscaler_nodes_count
cluster_autoscaler_unschedulable_pods_count
```

---

## 缩放故障排查

### HPA 不工作

```bash
# 🟢 诊断 HPA 状态
kubectl get hpa -n production
kubectl describe hpa api-server-hpa -n production

# 检查 metrics-server
kubectl get apiservice v1beta1.metrics.k8s.io
kubectl top pods -n production

# 检查自定义指标 API
kubectl get apiservice v1beta1.custom.metrics.k8s.io
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1 | jq '.resources[].name' | head -20

# 检查 Prometheus Adapter
kubectl -n monitoring logs -l app=prometheus-adapter --tail=50

# 常见问题:
# 1. <unknown> 指标: metrics-server 或 adapter 未部署
# 2. 无法缩放: 达到 maxReplicas 或 minReplicas
# 3. 频繁缩放: stabilizationWindow 太短
```

### KEDA 缩放异常

```bash
# 🟢 检查 KEDA 状态
kubectl get scaledobjects -A
kubectl describe scaledobject order-consumer-scaler -n production

# 检查 KEDA Operator
kubectl -n keda logs -l app=keda-operator --tail=50
kubectl -n keda logs -l app=keda-admission-webhooks --tail=50

# 检查触发器连接
# Kafka: 确认 bootstrapServers 可达
# Prometheus: 确认 serverAddress 可访问

# 常见问题:
# 1. 缩放到 0 后无法恢复: 检查 minReplicaCount
# 2. 缩放延迟: 调整 pollingInterval
# 3. 触发器错误: 检查 metadata 配置
```

### Cluster Autoscaler 问题

```bash
# 🟢 检查 CA 状态
kubectl -n kube-system logs -l app=cluster-autoscaler --tail=100

# 检查未调度 Pod
kubectl get pods -A --field-selector=status.phase=Pending
kubectl describe pod <pending-pod> -n <ns> | grep -A 10 Events

# 检查节点池状态
kubectl get nodes --show-labels | grep -E "(node-pool|karpenter)"

# 常见问题:
# 1. 节点不扩容: 检查 CA 配置/权限/节点池上限
# 2. 节点不缩容: 检查 PDB/annotation/利用率阈值
# 3. 扩容太慢: 调整 scan-interval
```

---

## 缩放安全与稳定性

### PodDisruptionBudget

```yaml
# 确保缩容/升级时不会全部下线
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-server-pdb
  namespace: production
spec:
  minAvailable: 2  # 至少保持 2 个可用
  # 或: maxUnavailable: 1
  selector:
    matchLabels:
      app: api-server
---
# 关键服务: 百分比保护
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: payment-pdb
spec:
  maxUnavailable: "10%"  # 最多 10% 不可用
  selector:
    matchLabels:
      app: payment-service
```

### 缩放稳定性配置

```yaml
# HPA 防震荡配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: stable-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  minReplicas: 3
  maxReplicas: 50
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60   # 扩容等待 1 分钟
      policies:
        - type: Percent
          value: 50                       # 每次最多扩 50%
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 600  # 缩容等待 10 分钟
      policies:
        - type: Percent
          value: 5                        # 每次最多缩 5%
          periodSeconds: 120
      selectPolicy: Min                   # 取最保守策略
```

---

## 成本与性能平衡

### 缩放策略成本影响

| 策略 | 成本影响 | 性能影响 | 适用 |
|------|----------|----------|------|
| 激进扩容 + 保守缩容 | 高 | 最佳响应 | 关键业务 |
| 平衡策略 | 中 | 良好 | 大多数服务 |
| 保守扩容 + 激进缩容 | 低 | 可能有延迟 | 成本敏感 |
| 定时缩放 | 最低 | 可预测 | 流量可预测 |

### 资源利用率目标

```yaml
# 生产环境推荐目标
# CPU: 60-70% 利用率（留余量应对突发）
# Memory: 70-80% 利用率（避免 OOM）

# HPA 目标设置
metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 65  # 65% 触发扩容
```

---

## 缩放告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: autoscaling-alerts
  namespace: monitoring
spec:
  groups:
    - name: autoscaling.rules
      rules:
        # HPA 达到上限
        - alert: HPAMaxedOut
          expr: |
            kube_horizontalpodautoscaler_status_current_replicas
            == kube_horizontalpodautoscaler_spec_max_replicas
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "HPA {{ $labels.horizontalpodautoscaler }} 已达到最大副本数"

        # 持续 Pending Pod
        - alert: PodsPendingTooLong
          expr: |
            kube_pod_status_phase{phase="Pending"} == 1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 10 分钟"

        # Cluster Autoscaler 失败
        - alert: ClusterAutoscalerFailing
          expr: |
            rate(cluster_autoscaler_failed_scale_ups_total[15m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Cluster Autoscaler 扩容失败"

        # KEDA 缩放器错误
        - alert: KEDAScalerError
          expr: |
            keda_scaler_errors > 0
          for: 5m
          labels:
            severity: warning
```

## Related

- [[01-集群基础/07-性能调优/index.md|性能调优]]
- [[13-生产运维/01-成本治理/index.md|成本治理]]
- [[09-可观测性/02-指标/index.md|指标 Metrics]]
