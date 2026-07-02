---
title: OpenTelemetry Collector 配置模式
description: 'OTel Collector 配置模式：Pipeline 设计 (receiver→processor→exporter)、批处理器/采样处理器、多租户配置、K8s 自动注入、性能调优'
summary: 'OTel Collector Pipeline、处理器配置、多租户与 K8s 自动注入'
category: observability
tags:
- opentelemetry
- otel-collector
- pipeline
- sampling
- auto-instrumentation
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- OpenTelemetry Collector 配置模式是什么
- 如何配置 OTel Collector Pipeline
trigger_keywords:
- OpenTelemetry Collector
- OTel Collector
- Pipeline
- receiver
- processor
- exporter
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# OpenTelemetry Collector 配置模式

## 概述

OpenTelemetry Collector 是厂商中立的遥测数据收集代理，支持 Traces、Metrics、Logs 三种信号类型。本文档涵盖 Pipeline 设计、处理器配置、多租户方案和 K8s 自动注入等生产级配置模式。

## 1. Pipeline 架构

### 1.1 基础 Pipeline 模式

```
┌──────────────────────────────────────────────────────────┐
│                  OTel Collector                           │
│                                                          │
│  Receivers         Processors         Exporters          │
│  ┌──────┐         ┌──────────┐       ┌──────────┐       │
│  │ OTLP │────────→│  batch   │──────→│ OTLP     │       │
│  └──────┘         │  memory  │       │ (Tempo)  │       │
│  ┌──────┐         │  limiter │       └──────────┘       │
│  │Jaeger│────────→│  filter  │       ┌──────────┐       │
│  └──────┘         │  k8sattr │──────→│Prometheus│       │
│  ┌──────┐         │  span    │       │ (Mimir)  │       │
│  │Prom  │────────→│  metrics │       └──────────┘       │
│  └──────┘         └──────────┘       ┌──────────┐       │
│  ┌──────┐                            │ Loki     │       │
│  │Fluent│───────────────────────────→│          │       │
│  └──────┘                            └──────────┘       │
└──────────────────────────────────────────────────────────┘
```

### 1.2 多 Pipeline 模式

```yaml
# otel-collector-config.yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318

  jaeger:
    protocols:
      thrift_compact:
        endpoint: 0.0.0.0:6831
      grpc:
        endpoint: 0.0.0.0:14250

  prometheus:
    config:
      scrape_configs:
      - job_name: 'kubernetes-pods'
        kubernetes_sd_configs:
        - role: pod
        relabel_configs:
        - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
          action: keep
          regex: true

  filelog:
    include: [/var/log/containers/*.log]
    exclude: [/var/log/containers/*_kube-system_*.log]
    operators:
    - type: container
      id: container-parser

processors:
  batch:
    timeout: 5s
    send_batch_size: 1024
    send_batch_max_size: 2048

  memory_limiter:
    check_interval: 1s
    limit_mib: 512
    spike_limit_mib: 128

  k8sattributes:
    auth_type: "serviceAccount"
    passthrough: false
    extract:
      metadata:
      - k8s.namespace.name
      - k8s.deployment.name
      - k8s.pod.name
      - k8s.pod.uid
      - k8s.node.name
      labels:
      - tag_name: app
        key: app.kubernetes.io/name
        from: pod
    pod_association:
    - sources:
      - from: resource_attribute
        name: k8s.pod.ip

  filter/traces:
    error_mode: ignore
    traces:
      span:
        - 'attributes["http.target"] == "/health"'
        - 'attributes["http.target"] == "/metrics"'

  attributes:
    actions:
    - key: environment
      value: production
      action: upsert
    - key: service.version
      from_attribute: app.kubernetes.io/version
      action: insert

exporters:
  otlp/tempo:
    endpoint: tempo-distributor.observability.svc:4317
    tls:
      insecure: true
    retry_on_failure:
      enabled: true
      initial_interval: 5s
      max_interval: 30s
      max_elapsed_time: 300s

  prometheusremotewrite/mimir:
    endpoint: http://mimir-distributor.observability.svc:8080/api/v1/push

  loki:
    endpoint: http://loki-gateway.observability.svc:3100/loki/api/v1/push

extensions:
  health_check:
    endpoint: 0.0.0.0:13133
  pprof:
    endpoint: 0.0.0.0:1888
  zpages:
    endpoint: 0.0.0.0:55679

service:
  extensions: [health_check, pprof, zpages]
  pipelines:
    traces:
      receivers: [otlp, jaeger]
      processors: [memory_limiter, k8sattributes, filter/traces, attributes, batch]
      exporters: [otlp/tempo]

    metrics:
      receivers: [otlp, prometheus]
      processors: [memory_limiter, k8sattributes, batch]
      exporters: [prometheusremotewrite/mimir]

    logs:
      receivers: [otlp, filelog]
      processors: [memory_limiter, k8sattributes, batch]
      exporters: [loki]
```

## 2. 处理器详解

### 2.1 Batch Processor

```yaml
processors:
  batch:
    # 发送超时时间
    timeout: 5s
    # 每批最小大小
    send_batch_size: 1024
    # 每批最大大小
    send_batch_max_size: 2048
    # 并发发送数量
    metadata_cardinality_limit: 1000
```

### 2.2 Memory Limiter Processor

```yaml
processors:
  memory_limiter:
    # 检查间隔
    check_interval: 1s
    # 内存限制（MB）
    limit_mib: 512
    # 突发内存限制
    spike_limit_mib: 128
    # 内存限制百分比（相对于系统内存）
    limit_percentage: 80
    spike_limit_percentage: 20
```

### 2.3 Tail Sampling Processor

```yaml
processors:
  tail_sampling:
    decision_wait: 10s
    num_traces: 100000
    expected_new_traces_per_sec: 1000
    policies:
    # 错误采样（100%）
    - name: errors
      type: status_code
      status_code:
        status_codes: [ERROR]

    # 慢请求采样（100%）
    - name: slow-traces
      type: latency
      latency:
        threshold_ms: 5000

    # 按 Service 采样
    - name: payment-service
      type: string_attribute
      string_attribute:
        key: service.name
        values: [payment-service]
      type: probabilistic
      probabilistic:
        sampling_percentage: 50

    # 默认采样率
    - name: default
      type: probabilistic
      probabilistic:
        sampling_percentage: 10
```

### 2.4 Span Metrics Processor

```yaml
processors:
  spanmetrics:
    metrics_exporter: prometheusremotewrite
    latency_histogram_buckets: [1ms, 5ms, 10ms, 25ms, 50ms, 100ms, 250ms, 500ms, 1s, 5s]
    dimensions:
    - name: http.method
    - name: http.status_code
    - name: service.name
    - name: service.namespace
    aggregation_temporality: "AGGREGATION_TEMPORALITY_CUMULATIVE"
    enable_open_census_bridge: true
```

## 3. 多租户配置

### 3.1 Header-based 多租户

```yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318

processors:
  routing:
    attribute_source: context
    from_attribute: X-Tenant-ID
    default_tenant: default
    table:
    - value: tenant-a
      exporters: [otlp/tempo-tenant-a]
    - value: tenant-b
      exporters: [otlp/tempo-tenant-b]

exporters:
  otlp/tempo-tenant-a:
    endpoint: tempo-tenant-a.observability.svc:4317
    headers:
      X-Scope-OrgID: tenant-a

  otlp/tempo-tenant-b:
    endpoint: tempo-tenant-b.observability.svc:4317
    headers:
      X-Scope-OrgID: tenant-b

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [memory_limiter, routing]
      exporters: [otlp/tempo-tenant-a, otlp/tempo-tenant-b]
```

### 3.2 命名空间隔离

```yaml
# 每个命名空间独立的 OTel Collector
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
  namespace: tenant-a
data:
  config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317

    processors:
      attributes:
        actions:
        - key: tenant
          value: tenant-a
          action: upsert
      batch:
        timeout: 5s

    exporters:
      otlp:
        endpoint: central-otel-collector.observability.svc:4317

    service:
      pipelines:
        traces:
          receivers: [otlp]
          processors: [attributes, batch]
          exporters: [otlp]
```

## 4. K8s 自动注入

### 4.1 OpenTelemetry Operator 部署

```bash
# 安装 OTel Operator
kubectl apply -f https://github.com/open-telemetry/opentelemetry-operator/releases/latest/download/opentelemetry-operator.yaml
```

### 4.2 Instrumentation CR

```yaml
apiVersion: opentelemetry.io/v1alpha1
kind: Instrumentation
metadata:
  name: instrumentation
  namespace: production
spec:
  exporter:
    endpoint: http://otel-collector.observability.svc:4317

  propagators:
  - tracecontext
  - baggage

  sampler:
    type: parentbased_traceidratio
    argument: "0.1"

  env:
  - name: OTEL_K8S_NAMESPACE
    valueFrom:
      fieldRef:
        apiVersion: v1
        fieldPath: metadata.namespace
  - name: OTEL_K8S_POD_NAME
    valueFrom:
      fieldRef:
        apiVersion: v1
        fieldPath: metadata.name
  - name: OTEL_K8S_NODE_NAME
    valueFrom:
      fieldRef:
        apiVersion: v1
        fieldPath: spec.nodeName

  java:
    image: ghcr.io/open-telemetry/opentelemetry-operator/autoinstrumentation-java:latest
    env:
    - name: OTEL_JAVAAGENT_ENABLED
      value: "true"

  python:
    image: ghcr.io/open-telemetry/opentelemetry-operator/autoinstrumentation-python:latest

  nodejs:
    image: ghcr.io/open-telemetry/opentelemetry-operator/autoinstrumentation-nodejs:latest
```

### 4.3 自动注入配置

```yaml
# Deployment 添加注解启用自动注入
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: production
spec:
  template:
    metadata:
      annotations:
        instrumentation.opentelemetry.io/inject-java: "true"
        instrumentation.opentelemetry.io/inject-python: "true"
        sidecar.opentelemetry.io/inject: "true"
    spec:
      containers:
      - name: app
        image: my-app:latest
```

## 5. 性能调优

### 5.1 Collector 资源配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: otel-collector
  namespace: observability
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: otel-collector
        resources:
          requests:
            cpu: "1"
            memory: 1Gi
          limits:
            cpu: "2"
            memory: 2Gi
        env:
        - name: GOGC
          value: "100"
        - name: GOMEMLIMIT
          value: "1800MiB"
```

### 5.2 队列和重试配置

```yaml
exporters:
  otlp/tempo:
    endpoint: tempo-distributor.observability.svc:4317
    sending_queue:
      enabled: true
      num_consumers: 10
      queue_size: 5000
      storage: file_storage
    retry_on_failure:
      enabled: true
      initial_interval: 5s
      max_interval: 30s
      max_elapsed_time: 300s

extensions:
  file_storage:
    directory: /var/lib/otel/storage
    timeout: 1s
```

### 5.3 Prometheus 监控

```yaml
# OTel Collector 自身指标暴露
service:
  telemetry:
    metrics:
      address: 0.0.0.0:8888
      level: detailed
    logs:
      level: info

# Prometheus ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: otel-collector
  namespace: observability
spec:
  selector:
    matchLabels:
      app: otel-collector
  endpoints:
  - port: metrics
    interval: 15s
```

## 6. 最佳实践

```
OTel Collector 配置检查清单：

□ 配置 memory_limiter 防止 OOM
□ 使用 batch processor 优化吞吐量
□ 配置 k8sattributes processor 自动添加 K8s 元数据
□ 使用 filter processor 过滤健康检查等噪音数据
□ 配置 retry 和 queue 机制保证可靠性
□ 使用 OTel Operator 实现自动注入
□ 配置多 Pipeline 分离不同信号类型
□ 部署 HPA 自动伸缩 Collector
□ 监控 Collector 自身指标
□ 使用 Tail Sampling 优化采样成本
```

## Related

- [[domain-06-observability/04-tracing/01-jaeger-production-deployment|Jaeger 生产部署]]
- [[domain-06-observability/04-tracing/02-grafana-tempo-tracing|Grafana Tempo]]

## See Also

- [OpenTelemetry Collector 文档](https://opentelemetry.io/docs/collector/)
- [OTel Operator 文档](https://opentelemetry.io/docs/kubernetes/operator/)
