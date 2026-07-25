---
title: KEDA + HPA 模型推理弹性伸缩
description: 基于自定义指标的 LLM 推理服务弹性伸缩
summary: 使用 KEDA 基于 Prometheus 指标（队列深度/GPU 利用率）实现推理服务弹性伸缩
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- keda
- hpa
- autoscaling
- inference
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- KEDA GPU 伸缩
- LLM 推理弹性
- 基于队列深度自动伸缩
trigger_keywords:
- keda
- hpa
- autoscaling
- queue-depth
- prometheus
prerequisites:
- hpa-basics
- prometheus-basics
authors:
- name: KUDIG Team
  role: contributor
---

# KEDA + HPA 模型推理弹性伸缩

## 1. 为什么需要 KEDA

原生 HPA 只支持 CPU/内存和自定义指标 API，而 KEDA 提供：

| 特性 | HPA | KEDA |
|------|-----|------|
| 指标源 | metrics-server | Prometheus/Kafka/Redis/自定义 |
| Scale to Zero | 不支持 | 支持 |
| 多触发器 | 有限 | 丰富 |
| 配置复杂度 | 中 | 低（声明式） |

## 2. KEDA ScaledObject — 基于 Prometheus 指标

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: vllm-autoscaler
  namespace: ai-inference
spec:
  scaleTargetRef:
    name: vllm-server           # 目标 Deployment
  minReplicaCount: 1            # 最小副本数
  maxReplicaCount: 8            # 最大副本数
  pollingInterval: 15           # 轮询间隔（秒）
  cooldownPeriod: 60            # 缩容冷却时间
  triggers:
    # 基于等待队列长度
    - type: prometheus
      metadata:
        serverAddress: http://prometheus.monitoring:9090
        metricName: vllm_pending_requests
        threshold: "10"         # 每实例最多 10 个排队
        query: |
          avg(vllm:num_requests_waiting{namespace="ai-inference"})
    # 基于 GPU 利用率
    - type: prometheus
      metadata:
        serverAddress: http://prometheus.monitoring:9090
        metricName: gpu_utilization
        threshold: "80"         # GPU 利用率超 80% 扩容
        query: |
          avg(DCGM_FI_DEV_GPU_UTIL{namespace="ai-inference"})
```

## 3. 多触发器组合

```yaml
triggers:
  # 触发器 1: Kafka 消息队列深度
  - type: kafka
    metadata:
      bootstrapServers: kafka.messaging:9092
      consumerGroup: inference-group
      topic: inference-requests
      lagThreshold: "100"
  # 触发器 2: 自定义 Prometheus 指标
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring:9090
      metricName: request_latency_p99
      threshold: "2000"         # P99 延迟超 2s
      query: |
        histogram_quantile(0.99,
          sum(rate(http_request_duration_ms_bucket{service="vllm"}[2m])) by (le)
        )
  # 触发器 3: CPU（后备）
  - type: cpu
    metadata:
      type: Utilization
      value: "70"
```

## 4. Scale to Zero 配置

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: batch-inference
  namespace: ai-inference
spec:
  scaleTargetRef:
    name: batch-processor
  minReplicaCount: 0            # 允许缩到 0
  maxReplicaCount: 4
  idleReplicaCount: 0           # 空闲时保持 0 副本
  cooldownPeriod: 300           # 5 分钟无请求后缩容到 0
  triggers:
    - type: kafka
      metadata:
        bootstrapServers: kafka.messaging:9092
        consumerGroup: batch-group
        topic: batch-jobs
        lagThreshold: "5"
```

## 5. GPU 感知弹性伸缩

```yaml
# 基于自定义 GPU 指标的 HPA
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: inference-gpu-hpa
  namespace: ai-inference
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: inference-server
  minReplicas: 1
  maxReplicas: 8
  metrics:
    # GPU 利用率指标
    - type: Pods
      pods:
        metric:
          name: gpu_utilization
          selector:
            matchLabels:
              app: inference-server
        target:
          type: AverageValue
          averageValue: "80"
    # 推理 QPS
    - type: Pods
      pods:
        metric:
          name: inference_requests_per_second
        target:
          type: AverageValue
          averageValue: "20"     # 每实例 20 QPS
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Pods
          value: 2               # 每次最多加 2 个
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300  # 5 分钟稳定后缩容
      policies:
        - type: Pods
          value: 1               # 每次最多减 1 个
          periodSeconds: 120
```

## 6. Prometheus Adapter 配置

```yaml
# 将自定义指标暴露给 HPA
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-adapter-config
  namespace: monitoring
data:
  config.yaml: |
    rules:
      - seriesQuery: 'DCGM_FI_DEV_GPU_UTIL{namespace!="",pod!=""}'
        resources:
          overrides:
            namespace: {resource: "namespace"}
            pod: {resource: "pod"}
        name:
          matches: "^(.*)_UTIL"
          as: "gpu_utilization"
        metricsQuery: 'avg(DCGM_FI_DEV_GPU_UTIL{<<.LabelMatchers>>}) by (<<.GroupBy>>)'
      - seriesQuery: 'vllm:num_requests_running{namespace!="",pod!=""}'
        resources:
          overrides:
            namespace: {resource: "namespace"}
            pod: {resource: "pod"}
        name:
          as: "inference_requests_per_second"
        metricsQuery: 'rate(vllm:num_requests_running{<<.LabelMatchers>>}[2m])'
```

## 7. 触发器认证

```yaml
# 如果 Prometheus 需要 TLS/认证
apiVersion: v1
kind: Secret
metadata:
  name: prometheus-auth
  namespace: ai-inference
data:
  TLS_CA_CERT: <base64>
  TLS_CLIENT_CERT: <base64>
  TLS_CLIENT_KEY: <base64>
---
apiVersion: keda.sh/v1alpha1
kind: TriggerAuthentication
metadata:
  name: prometheus-trigger-auth
  namespace: ai-inference
spec:
  secretTargetRef:
    - parameter: tlsCACert
      name: prometheus-auth
      key: TLS_CA_CERT
    - parameter: tlsClientCert
      name: prometheus-auth
      key: TLS_CLIENT_CERT
    - parameter: tlsClientKey
      name: prometheus-auth
      key: TLS_CLIENT_KEY
```

## 8. 扩容行为优化

```yaml
behavior:
  scaleUp:
    stabilizationWindowSeconds: 0      # 立即扩容
    selectPolicy: Max                  # 选择最大扩容策略
    policies:
      - type: Percent
        value: 100                     # 可以翻倍扩容
        periodSeconds: 30
      - type: Pods
        value: 4                       # 或最多加 4 个
        periodSeconds: 30
  scaleDown:
    stabilizationWindowSeconds: 300    # 5 分钟后才缩容
    selectPolicy: Min                  # 保守缩容
    policies:
      - type: Pods
        value: 1                       # 每次减 1
        periodSeconds: 120
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 扩容快缩容慢 | 避免请求高峰时缩容 |
| 多指标组合 | 队列深度 + GPU 利用率 |
| 设置 `cooldownPeriod` | 避免频繁伸缩 |
| 监控 KEDA 日志 | 确认触发器正常工作 |
| Scale to Zero 适用场景 | 批处理/非实时推理 |
| 模型预热 | 新 Pod 启动需要时间加载模型 |

## Related

- [[03-清单模式/08-韧性模式/02-hpa-advanced-patterns|HPA 高级模式]]
- [[03-清单模式/07-AI-ML模式/03-vllm-deployment-manifest|vLLM 部署]]

## See Also

- [KEDA 文档](https://keda.sh/docs/)
- [Prometheus Adapter](https://github.com/kubernetes-sigs/prometheus-adapter)
- [GPU 自适应伸缩](https://keda.sh/docs/2.0/scalers/prometheus/)

<!-- risk-assessed -->
