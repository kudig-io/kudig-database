---
title: Distributed Tracing Production Patterns — Sampling, Trace Analysis, and SLO Integration
description: 分布式追踪生产模式 — 采样策略、Trace 分析、SLO 集成、跨语言传播、成本优化、故障定位
summary: 生产环境分布式追踪的采样策略、分析模式与 SLO 集成实践
category: practice
tags:
- distributed-tracing
- sampling
- opentelemetry
- trace-analysis
- slo
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: observability
---
# 分布式追踪生产模式

> 采样策略、Trace 分析与 SLO 集成的生产实践。

## 采样策略

### 采样类型对比

| 策略 | 原理 | 优点 | 缺点 | 适用 |
|------|------|------|------|------|
| 头部采样（Head） | 入口决定采样 | 简单、低开销 | 可能错过异常 | 低流量服务 |
| 尾部采样（Tail） | 完成后决定 | 保留所有异常 | 需缓冲、高内存 | 高流量关键服务 |
| 比率采样 | 固定百分比 | 均匀分布 | 低频错误可能丢失 | 通用 |
| 自适应采样 | 动态调整比率 | 平衡成本与覆盖 | 实现复杂 | 大规模 |

### OTel Collector 尾部采样

```yaml
# otel-collector-config.yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318

processors:
  # 尾部采样（保留所有错误和慢请求）
  tail_sampling:
    decision_wait: 10s
    num_traces: 100000
    expected_new_traces_per_sec: 1000
    policies:
      # 保留所有错误 Trace
      - name: errors
        type: status_code
        status_code:
          status_codes: [ERROR]
      # 保留慢请求（> 2s）
      - name: slow-traces
        type: latency
        latency:
          threshold_ms: 2000
      # 保留特定属性
      - name: critical-operations
        type: string_attribute
        string_attribute:
          key: http.route
          values: ["/api/v1/payments", "/api/v1/orders"]
      # 基础比率采样（10%）
      - name: base-rate
        type: probabilistic
        probabilistic:
          sampling_percentage: 10

  # 资源属性丰富
  resource:
    attributes:
      - key: deployment.environment
        value: production
        action: upsert

  # 批量处理
  batch:
    timeout: 5s
    send_batch_size: 1024
    send_batch_max_size: 2048

exporters:
  otlp/tempo:
    endpoint: tempo.monitoring:4317
    tls:
      insecure: true
  # 采样前的指标（用于监控采样效果）
  prometheus:
    endpoint: 0.0.0.0:8889

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [resource, tail_sampling, batch]
      exporters: [otlp/tempo]
```

### 应用端采样配置

```yaml
# Kubernetes Deployment 环境变量
env:
  - name: OTEL_TRACES_SAMPLER
    value: "parentbased_traceidratio"
  - name: OTEL_TRACES_SAMPLER_ARG
    value: "0.1"  # 10% 采样率
  - name: OTEL_EXPORTER_OTLP_ENDPOINT
    value: "http://otel-collector.monitoring:4317"
  - name: OTEL_SERVICE_NAME
    value: "order-service"
  - name: OTEL_RESOURCE_ATTRIBUTES
    value: "deployment.environment=production,service.version=2.1.0"
```

## Trace 分析模式

### 关键查询（Tempo/Grafana）

```
# 查找慢请求
{ duration > 2s && service.name = "order-service" }

# 查找错误 Trace
{ status = error && service.name = "payment-service" }

# 查找特定用户的请求
{ span.http.request.header.x-user-id = "user-12345" }

# 查找特定路由
{ span.http.route = "/api/v1/orders" && duration > 1s }

# 跨服务关联
{ trace:id = "abc123def456" }
```

### Trace 驱动的性能分析

```
请求: POST /api/v1/orders (总耗时 3.2s)
├── [order-service] HTTP Handler: 3200ms
│   ├── [order-service] DB Query (SELECT inventory): 150ms
│   ├── [payment-service] gRPC Charge: 2800ms ← 瓶颈
│   │   ├── [payment-service] Stripe API Call: 2500ms ← 外部依赖
│   │   └── [payment-service] DB Write: 200ms
│   ├── [notification-service] gRPC Notify: 100ms
│   └── [order-service] DB Write: 150ms
```

## SLO 集成

### 基于 Trace 的 SLI

```yaml
# 使用 Trace 数据计算 SLI
# Grafana Tempo + Metrics Generator
apiVersion: v1
kind: ConfigMap
metadata:
  name: tempo-config
  namespace: monitoring
data:
  tempo.yaml: |
    metrics_generator:
      registry:
        external_labels:
          source: tempo
      storage:
        path: /var/tempo/generator/wal
        remote_write:
          - url: http://prometheus:9090/api/v1/write
      processor:
        service_graphs:
          dimensions:
            - http.method
            - http.status_code
        span_metrics:
          dimensions:
            - http.route
            - http.method
          # 生成延迟直方图
          histogram_buckets: [0.002, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10]
```

### 从 Trace 生成的指标

```promql
# P99 延迟（从 Trace 生成）
histogram_quantile(0.99,
  sum(rate(traces_spanmetrics_duration_milliseconds_bucket{
    service_name="order-service",
    http_route="/api/v1/orders"
  }[5m])) by (le)
)

# 错误率
sum(rate(traces_spanmetrics_calls_total{
  service_name="order-service",
  status_code="STATUS_CODE_ERROR"
}[5m]))
/
sum(rate(traces_spanmetrics_calls_total{
  service_name="order-service"
}[5m]))

# 服务依赖图（Service Graph）
sum(rate(traces_service_graph_request_total[5m])) by (client, server)
```

## 跨语言传播

### W3C TraceContext 传播

```
HTTP Header:
traceparent: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
             │  │                                │                │
             版本 trace-id (32 hex)              parent-id (16)  flags

tracestate: vendor1=value1,vendor2=value2
```

### 各语言 SDK 配置

| 语言 | SDK | 自动传播 | 配置 |
|------|-----|----------|------|
| Go | otel-go | net/http 拦截器 | `otelhttp.NewHandler()` |
| Java | otel-javaagent | 自动（-javaagent） | 零代码 |
| Node.js | otel-node | http/express 插件 | `getNodeAutoInstrumentations()` |
| Python | otel-python | Flask/Django 插件 | `opentelemetry-instrument` |
| Rust | opentelemetry-rust | tower/axum 中间件 | 手动集成 |

## 成本优化

| 策略 | 节省 | 实现 |
|------|------|------|
| 尾部采样（仅保留错误+慢） | 60-80% | OTel Collector |
| 降低基础采样率 | 50-90% | 环境变量 |
| 缩短保留期 | 30-50% | Tempo 配置 |
| 属性裁剪 | 10-20% | Processor 过滤 |
| 非生产环境低采样 | 90%+ | 环境区分 |

```yaml
# 属性裁剪（减少存储）
processors:
  attributes:
    actions:
      - key: http.request.header.authorization
        action: delete  # 删除敏感头
      - key: db.statement
        action: update
        value: "[REDACTED]"  # 脱敏 SQL
```

## 故障排查

| 症状 | 原因 | 排查 |
|------|------|------|
| Trace 缺失 | 采样率过低/传播断裂 | 检查 traceparent header |
| 延迟数据不准 | 时钟不同步 | NTP 同步 |
| 服务名缺失 | OTEL_SERVICE_NAME 未设置 | 检查环境变量 |
| Collector OOM | 尾部采样缓冲过大 | 减小 num_traces |
| 存储成本高 | 采样率过高/保留期过长 | 调整策略 |

```bash
# 验证 Trace 传播
kubectl exec -it frontend-xxx -n production -- \
  curl -v http://api-server:8080/api/v1/status 2>&1 | grep traceparent

# OTel Collector 状态
kubectl logs -n monitoring -l app=otel-collector --tail=50
curl -s http://otel-collector:8888/metrics | grep otelcol_receiver_accepted_spans
```

---

## 自适应采样实现

### 基于服务粒度的动态采样

```yaml
# OTel Collector 自适应采样配置
processors:
  # 基于服务粒度的采样策略
  tail_sampling:
    decision_wait: 15s
    num_traces: 200000
    expected_new_traces_per_sec: 2000
    policies:
      # 关键服务: 高采样率
      - name: critical-services
        type: and
        and:
          and_sub_policy:
            - name: service-filter
              type: string_attribute
              string_attribute:
                key: service.name
                values: ["payment-service", "auth-service", "order-service"]
            - name: high-rate
              type: probabilistic
              probabilistic:
                sampling_percentage: 50

      # 普通服务: 低采样率
      - name: standard-services
        type: probabilistic
        probabilistic:
          sampling_percentage: 5

      # 所有错误: 100% 保留
      - name: all-errors
        type: status_code
        status_code:
          status_codes: [ERROR]

      # 慢请求: 100% 保留
      - name: latency-slo
        type: latency
        latency:
          threshold_ms: 1000

      # 特定用户调试: 100% 保留
      - name: debug-users
        type: string_attribute
        string_attribute:
          key: http.request.header.x-debug-trace
          values: ["true"]
```

### 采样率动态调整脚本

```bash
#!/bin/bash
# 🟢 根据流量自动调整采样率
# 用于 CronJob 定期执行

PROM_URL="http://prometheus.monitoring:9090"
COLLECTOR_CONFIG="/etc/otel-collector/config.yaml"

# 获取当前 QPS
CURRENT_QPS=$(curl -s "$PROM_URL/api/v1/query?query=sum(rate(http_requests_total[5m]))" | \
  jq '.data.result[0].value[1]' | cut -d'"' -f2 | cut -d'.' -f1)

# 根据 QPS 决定采样率
if [ "$CURRENT_QPS" -gt 10000 ]; then
  SAMPLE_RATE="0.01"  # 1%
elif [ "$CURRENT_QPS" -gt 5000 ]; then
  SAMPLE_RATE="0.05"  # 5%
elif [ "$CURRENT_QPS" -gt 1000 ]; then
  SAMPLE_RATE="0.1"   # 10%
else
  SAMPLE_RATE="0.5"   # 50%
fi

echo "当前 QPS: $CURRENT_QPS, 采样率: $SAMPLE_RATE"
# 更新 Collector 配置并 reload
```

---

## Trace 关联日志与指标

### 日志中注入 TraceID

```yaml
# 应用 Deployment 配置
env:
  # 日志格式包含 trace_id
  - name: LOG_FORMAT
    value: "json"
  - name: OTEL_LOGS_EXPORTER
    value: "otlp"
  - name: OTEL_EXPORTER_OTLP_LOGS_ENDPOINT
    value: "http://otel-collector.monitoring:4317"
---
# OTel Collector 日志处理
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317

processors:
  # 日志中注入 trace 关联
  transform:
    log_statements:
      - context: log
        statements:
          - set(attributes["trace_id"], TraceID)
          - set(attributes["span_id"], SpanID)
          - set(attributes["service.name"], resource.attributes["service.name"])

exporters:
  loki:
    endpoint: http://loki.monitoring:3100/loki/api/v1/push
    labels:
      attributes:
        service_name: "service.name"
        trace_id: "trace_id"
```

### Grafana 三支柱关联查询

```promql
# 从指标发现异常 → 跳转 Trace
# 1. 发现 P99 延迟突增
histogram_quantile(0.99,
  sum(rate(http_request_duration_seconds_bucket{
    service="order-service"
  }[5m])) by (le)
) > 2

# 2. 跳转 Tempo 查询慢请求
# TraceQL:
# { duration > 2s && service.name = "order-service" }

# 3. 从 Trace 跳转关联日志
# LogQL:
# {service_name="order-service"} |= "trace_id=abc123"
```

### 服务拓扑图生成

```yaml
# Tempo Metrics Generator 生成服务图
metrics_generator:
  processor:
    service_graphs:
      dimensions:
        - http.method
        - http.status_code
        - http.route
      # 虚拟节点（外部依赖）
      virtual_node_dimensions:
        - db.system
        - rpc.system
      # 过期时间
      max_items: 10000
      expire_delay: 30s
```

```promql
# 服务依赖图查询
# 请求速率
sum(rate(traces_service_graph_request_total[5m])) by (client, server)

# 错误率
sum(rate(traces_service_graph_request_failed_total[5m])) by (client, server)
/
sum(rate(traces_service_graph_request_total[5m])) by (client, server)

# P99 延迟
histogram_quantile(0.99,
  sum(rate(traces_service_graph_request_server_seconds_bucket[5m])) by (le, client, server)
)
```

---

## 多集群追踪架构

### 架构设计

```
Cluster-A (CN)              Cluster-B (US)
┌─────────────────┐    ┌─────────────────┐
│ OTel Collector  │    │ OTel Collector  │
│ (DaemonSet)     │    │ (DaemonSet)     │
└────────┬────────┘    └────────┬────────┘
         │                       │
         ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│ OTel Gateway    │    │ OTel Gateway    │
│ (Deployment)    │    │ (Deployment)    │
└────────┬────────┘    └────────┬────────┘
         │                       │
         └───────────┬───────────┘
                     ▼
         ┌─────────────────┐
         │ Global Tempo    │
         │ (多租户)       │
         └─────────────────┘
```

### 多集群 Collector 配置

```yaml
# Gateway Collector (每个集群一个)
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317

processors:
  # 添加集群标识
  resource:
    attributes:
      - key: cluster.name
        value: "prod-cn-shanghai"
        action: upsert
      - key: cluster.region
        value: "cn-shanghai"
        action: upsert

  # 尾部采样（在 Gateway 层做）
  tail_sampling:
    decision_wait: 20s  # 跨集群需更长等待
    num_traces: 500000
    policies:
      - name: errors
        type: status_code
        status_code:
          status_codes: [ERROR]
      - name: slow
        type: latency
        latency:
          threshold_ms: 2000
      - name: base
        type: probabilistic
        probabilistic:
          sampling_percentage: 10

exporters:
  otlp/global-tempo:
    endpoint: tempo.global-monitoring:4317
    headers:
      x-scope-orgid: "prod-cn"  # 多租户标识
    retry_on_failure:
      enabled: true
      max_elapsed_time: 60s
    sending_queue:
      enabled: true
      num_consumers: 20
      queue_size: 5000
```

---

## 性能调优与容量规划

### Collector 资源规划

| 流量规模 | Spans/sec | Collector 副本 | CPU/副本 | Memory/副本 | 说明 |
|----------|-----------|------------|---------|------------|------|
| 小型 | < 1K | 2 | 500m | 512Mi | 头部采样 |
| 中型 | 1K-10K | 3 | 1000m | 2Gi | 尾部采样 |
| 大型 | 10K-100K | 5-10 | 2000m | 4Gi | 尾部+Gateway |
| 超大型 | > 100K | 10+ | 4000m | 8Gi | 分层架构 |

### 存储容量估算

```bash
# 🟢 估算 Tempo 存储需求
# 公式: 存储 = spans/sec × 平均 span 大小 × 保留时间 × 压缩比

SPANS_PER_SEC=5000
AVG_SPAN_BYTES=1500    # 平均 1.5KB/span
RETENTION_HOURS=72     # 3 天保留
COMPRESSION_RATIO=0.15 # 压缩后 15%

STORAGE_GB=$(echo "$SPANS_PER_SEC * $AVG_SPAN_BYTES * $RETENTION_HOURS * 3600 * $COMPRESSION_RATIO / 1024/1024/1024" | bc)
echo "预估存储: ${STORAGE_GB} GB"
```

### Collector 性能监控

```yaml
# PrometheusRule: OTel Collector 告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: otel-collector-alerts
  namespace: monitoring
spec:
  groups:
    - name: otel.rules
      rules:
        - alert: CollectorHighQueueSize
          expr: otelcol_exporter_queue_size > 4000
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Collector 发送队列积压"

        - alert: CollectorDroppingSpans
          expr: rate(otelcol_processor_dropped_spans[5m]) > 100
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Collector 正在丢弃 Spans"

        - alert: CollectorHighMemory
          expr: |
            container_memory_working_set_bytes{container="otel-collector"}
            / container_spec_memory_limit_bytes{container="otel-collector"} > 0.85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Collector 内存使用超过 85%"
```

---

## 异常检测与智能分析

### 基于 Trace 的异常检测

```promql
# 延迟异常检测（与历史基线对比）
# 当前 P99 vs 7 天同时段 P99
(
  histogram_quantile(0.99, sum(rate(traces_spanmetrics_duration_milliseconds_bucket{
    service_name="order-service"
  }[5m])) by (le))
  /
  histogram_quantile(0.99, sum(rate(traces_spanmetrics_duration_milliseconds_bucket{
    service_name="order-service"
  }[5m] offset 7d)) by (le))
) > 3  # 超过 3 倍则告警

# 错误率突增
(
  sum(rate(traces_spanmetrics_calls_total{
    service_name="order-service", status_code="STATUS_CODE_ERROR"
  }[5m]))
  /
  sum(rate(traces_spanmetrics_calls_total{service_name="order-service"}[5m]))
) > 0.05  # 错误率 > 5%
```

### Trace 质量监控

| 指标 | 健康值 | 告警值 | 说明 |
|------|---------|---------|------|
| 采样覆盖率 | > 5% | < 1% | 确保有足够样本 |
| Trace 完整性 | > 95% | < 80% | 无断裂 Trace |
| 传播成功率 | > 99% | < 95% | traceparent 传递 |
| Collector 丢弃率 | < 0.1% | > 1% | 不应丢弃数据 |
| 端到端延迟 | < 5s | > 30s | 从产生到可查询 |

---

## 生产最佳实践总结

| 维度 | 建议 | 说明 |
|------|------|------|
| 采样策略 | 尾部采样 + 错误/慢请求 100% | 不丢失关键信息 |
| 传播 | W3C TraceContext | 跨语言标准 |
| 关联 | Trace + Log + Metric 三支柱 | TraceID 贯穿 |
| 存储 | 热数据 3d + 冷数据 30d | 成本与查询平衡 |
| 多集群 | Gateway 汇聚 + 多租户 | 全局视图 |
| 成本 | 属性裁剪 + 环境区分 | 降低 60-80% |
| 监控 | Collector 自身指标 + 告警 | 确保管道健康 |

## Related

- [[09-可观测性/04-链路追踪/index.md|链路追踪]]
- [[09-可观测性/04-链路追踪/05-otel-collector-deep-configuration.md|OTel Collector]]
- [[09-可观测性/06-SLO-SLI/index.md|SLO/SLI]]
