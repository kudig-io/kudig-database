---
title: Grafana Tempo 分布式追踪
description: 'Grafana Tempo：部署架构 (Single/Microservices)、TraceQL 查询语言、与 Loki/Mimir 联动、搜索性能优化、Object Storage 后端配置'
summary: 'Tempo 部署架构、TraceQL、Grafana 全家桶联动与存储优化'
category: observability
tags:
- grafana-tempo
- distributed-tracing
- traceql
- object-storage
- grafana-stack
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
- Grafana Tempo 是什么
- 如何部署 Grafana Tempo
trigger_keywords:
- Grafana Tempo
- TraceQL
- 分布式追踪
- Object Storage
- Grafana Stack
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Grafana Tempo 分布式追踪

## 概述

Grafana Tempo 是 Grafana Labs 开源的高扩展性分布式追踪后端，专为低成本、高吞吐量设计。与 Jaeger 相比，Tempo 不需要独立的索引存储，直接使用 Object Storage 存储 Trace 数据，大幅降低运营成本。

## 1. 部署架构

### 1.1 Single Binary 模式（开发/小规模）

```
┌─────────────────────────────┐
│      Tempo (Single)         │
│  ┌───────┐ ┌─────┐ ┌────┐  │
│  │Distributor│ │Ingester│ │Query│  │
│  └───────┘ └─────┘ └────┘  │
│           │                  │
│           ▼                  │
│     ┌─────────┐              │
│     │ Object  │              │
│     │ Storage │              │
│     └─────────┘              │
└─────────────────────────────┘
```

### 1.2 Microservices 模式（生产推荐）

```
┌──────────────────────────────────────────────────────────────┐
│                  Tempo Microservices                         │
│                                                              │
│  ┌─────────────────────────────────────────────────┐        │
│  │              Distributor (Deployment)            │        │
│  │  接收 Trace → 验证 → 按 hash 分发到 Ingester   │        │
│  └───────────────────────┬─────────────────────────┘        │
│                          ▼                                   │
│  ┌─────────────────────────────────────────────────┐        │
│  │              Ingester (StatefulSet)              │        │
│  │  接收 Trace → 写入 WAL → 定期 Flush 到 Storage  │        │
│  │  副本因子: 3 (推荐)                              │        │
│  └───────────────────────┬─────────────────────────┘        │
│                          ▼                                   │
│  ┌─────────────────────────────────────────────────┐        │
│  │              Compactor (Deployment)              │        │
│  │  合并 Block → 压缩 → 清理过期数据               │        │
│  └───────────────────────┬─────────────────────────┘        │
│                          ▼                                   │
│  ┌─────────────────────────────────────────────────┐        │
│  │              Querier (Deployment)                │        │
│  │  接收查询 → 从 Storage/Ingester 读取 → 返回结果  │        │
│  └───────────────────────┬─────────────────────────┘        │
│                          ▼                                   │
│  ┌─────────────────────────────────────────────────┐        │
│  │              Query Frontend (Deployment)          │        │
│  │  查询缓存 → 查询拆分 → 并行查询                 │        │
│  └─────────────────────────────────────────────────┘        │
│                          ▼                                   │
│  ┌─────────────────────────────────────────────────┐        │
│  │              Object Storage                      │        │
│  │  S3 / GCS / Azure Blob / MinIO                   │        │
│  └─────────────────────────────────────────────────┘        │
└──────────────────────────────────────────────────────────────┘
```

## 2. 生产部署配置

### 2.1 Helm 部署

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Grafana Helm 仓库
helm repo add grafana https://grafana.github.io/helm-charts
helm repo update

# 生产模式部署
helm install tempo grafana/tempo-distributed \
  --namespace observability \
  --create-namespace \
  -f tempo-values.yaml
```
### 2.2 Helm Values（生产配置）

```yaml
# tempo-values.yaml
global:
  clusterDomain: cluster.local

distributor:
  replicas: 3
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
    limits:
      cpu: "2"
      memory: 2Gi
  config:
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: "0.0.0.0:4317"
          http:
            endpoint: "0.0.0.0:4318"
      jaeger:
        protocols:
          thrift_compact:
            endpoint: "0.0.0.0:6831"
          thrift_binary:
            endpoint: "0.0.0.0:6832"
          grpc:
            endpoint: "0.0.0.0:14250"
      zipkin:
        endpoint: "0.0.0.0:9411"

ingester:
  replicas: 3
  resources:
    requests:
      cpu: "1"
      memory: 2Gi
    limits:
      cpu: "2"
      memory: 4Gi
  persistence:
    enabled: true
    storageClass: fast-ssd
    size: 50Gi
  config:
    trace_idle_period: 10s
    max_block_bytes: 1048576  # 1MB
    max_block_duration: 30m
    complete_block_timeout: 3m
    flush_check_period: 5s

compactor:
  replicas: 1
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
    limits:
      cpu: "1"
      memory: 1Gi
  config:
    compaction:
      block_retention: 720h  # 30 天
      compacted_block_retention: 10m
      compaction_window: 1h
      max_compaction_objects: 6000000
      max_block_bytes: 107374182400  # 100GB

querier:
  replicas: 3
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
    limits:
      cpu: "2"
      memory: 2Gi
  config:
    frontend_worker:
      frontend_address: tempo-query-frontend:9095

query_frontend:
  replicas: 2
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
    limits:
      cpu: "1"
      memory: 1Gi
  config:
    max_retries: 2
    search:
      max_duration: 0  # 不限制搜索时间范围
      default_result_limit: 20
      max_result_limit: 0

storage:
  trace:
    backend: s3
    s3:
      bucket: tempo-traces
      endpoint: minio.observability.svc:9000
      access_key: ${MINIO_ACCESS_KEY}
      secret_key: ${MINIO_SECRET_KEY}
      insecure: true
    wal:
      path: /var/tempo/wal
    local:
      path: /var/tempo/blocks
    pool:
      max_workers: 100
      queue_depth: 10000
```

## 3. TraceQL 查询语言

### 3.1 基础查询

```traceql
# 按 Service 名称查询
{ resource.service.name = "payment-service" }

# 按 Span 名称查询
{ name = "processPayment" }

# 按状态查询
{ status = error }

# 组合条件
{ resource.service.name = "payment-service" && status = error }

# 按持续时间查询
{ duration > 1s }

# 按属性查询
{ span.http.method = "POST" && span.http.status_code = 500 }
```

### 3.2 结构化查询

```traceql
# 查找包含错误的 Trace
{ resource.service.name = "payment-service" } >> { status = error }

# 查找父 Span 慢、子 Span 正常的情况
{ duration > 5s } >> { duration < 100ms }

# 查找特定路径的 Trace
{ resource.service.name = "api-gateway" }
  >> { resource.service.name = "order-service" }
  >> { resource.service.name = "payment-service" }

# 聚合查询
{ resource.service.name = "payment-service" }
  | avg(duration) > 500ms
```

### 3.3 高级查询

```traceql
# 按环境过滤
{ resource.deployment.environment = "production" && resource.service.name = "api-gateway" }

# 查找跨服务调用链
{ resource.service.name = "api-gateway" && span.http.target = "/api/orders" }
  >> { resource.service.name = "order-service" }
  >> { resource.service.name = "inventory-service" }

# 按标签过滤
{ resource.k8s.namespace.name = "production" && resource.k8s.pod.name =~ "payment-.*" }

# 按时间范围查询
{ resource.service.name = "payment-service" && duration > 2s }
  | by(resource.service.name)
```

## 4. Grafana 全家桶联动

### 4.1 Tempo + Loki 联动

```yaml
# Grafana 数据源配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasources
  namespace: observability
data:
  datasources.yaml: |
    apiVersion: 1
    datasources:
    - name: Tempo
      type: tempo
      access: proxy
      url: http://tempo-query-frontend.observability.svc:3100
      uid: tempo
      jsonData:
        tracesToLogsV2:
          datasourceUid: loki
          filterByTraceID: true
          filterBySpanID: true
        tracesToMetrics:
          datasourceUid: prometheus
        serviceMap:
          datasourceUid: prometheus
        nodeGraph:
          enabled: true
        lokiSearch:
          datasourceUid: loki

    - name: Loki
      type: loki
      access: proxy
      url: http://loki-gateway.observability.svc:3100
      uid: loki
      jsonData:
        derivedFields:
        - datasourceUid: tempo
          matcherRegex: "trace_id=(\\w+)"
          name: TraceID
          url: "$${__value.raw}"
```

### 4.2 Tempo + Mimir 联动（Traces to Metrics）

```yaml
# 使用 Span Metrics 生成指标
# 在 OTel Collector 中配置 spanmetrics connector
connectors:
  spanmetrics:
    histogram:
      explicit:
        buckets: [5ms, 10ms, 25ms, 50ms, 100ms, 250ms, 500ms, 1s, 5s, 10s]
    dimensions:
    - name: http.method
    - name: http.status_code
    - name: service.name
    temporality: cumulative

exporters:
  prometheus/remotewrite:
    endpoint: http://mimir-distributor.observability.svc:8080/api/v1/push

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [otlp/tempo, spanmetrics]
    metrics/spanmetrics:
      receivers: [spanmetrics]
      exporters: [prometheus/remotewrite]
```

## 5. 搜索性能优化

### 5.1 搜索配置优化

```yaml
# tempo-config.yaml 搜索优化
query_frontend:
  search:
    max_duration: 0
    default_result_limit: 20
    max_result_limit: 0
    concurrent_jobs: 1000
    target_bytes_per_job: 10485760  # 10MB
    search_recent_trace: true
    ingester:
      search_target_bytes_per_job: 1048576  # 1MB
    default_search_filter: "{resource.service.name=~\".*\"}"

querier:
  search:
    prefer_self: 10
    external_endpoints: []
    external_hedge_requests_at: 5s
    external_hedge_requests_up_to: 2
```

### 5.2 缓存配置

```yaml
# 配置 Redis 缓存
query_frontend:
  cache:
    type: redis
    redis:
      endpoint: redis.observability.svc:6379
      expiration: 1h
      db: 0

ingester:
  cache:
    type: redis
    redis:
      endpoint: redis.observability.svc:6379
      expiration: 30m
      db: 1
```

## 6. Object Storage 配置

### 6.1 S3 配置

```yaml
storage:
  trace:
    backend: s3
    s3:
      bucket: tempo-traces
      endpoint: s3.ap-northeast-1.amazonaws.com
      region: ap-northeast-1
      access_key: ${AWS_ACCESS_KEY_ID}
      secret_key: ${AWS_SECRET_ACCESS_KEY}
      insecure: false
      part_size: 5242880  # 5MB
      hedge_requests_at: 500ms
      hedge_requests_up_to: 2
```

### 6.2 GCS 配置

```yaml
storage:
  trace:
    backend: gcs
    gcs:
      bucket_name: tempo-traces
      credentials_file: /etc/gcp/credentials.json
      prefix: traces/
```

### 6.3 MinIO 配置

```yaml
storage:
  trace:
    backend: s3
    s3:
      bucket: tempo-traces
      endpoint: minio.observability.svc:9000
      access_key: ${MINIO_ACCESS_KEY}
      secret_key: ${MINIO_SECRET_KEY}
      insecure: true
      forcepathstyle: true
```

## 7. 最佳实践

```
Tempo 生产部署检查清单：

□ 使用 Microservices 模式部署
□ 配置 Object Storage 作为后端（S3/GCS/MinIO）
□ 设置合适的 Block 保留时间
□ 配置 Ingester 副本因子（推荐 3）
□ 启用查询缓存（Redis）
□ 配置 TraceQL 搜索优化
□ 集成 Loki 实现 Traces to Logs
□ 集成 Mimir 实现 Traces to Metrics
□ 配置 Span Metrics 生成 RED 指标
□ 监控 Tempo 组件健康状态
```

## Related

- [[domain-06-observability/链路追踪/01-jaeger-production-deployment|Jaeger 生产部署]]
- [[domain-06-observability/链路追踪/03-opentelemetry-collector-patterns|OTel Collector 配置模式]]

## See Also

- [Grafana Tempo 文档](https://grafana.com/docs/tempo/latest/)
- [TraceQL 文档](https://grafana.com/docs/tempo/latest/traceql/)


<!-- risk-assessed -->
