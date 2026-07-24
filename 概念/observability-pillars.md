---
title: Observability Pillars
description: Observability Pillars — Kubernetes 生产运维知识库
summary: Observability Pillars — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- observability
- metrics
- logging
- tracing
- prometheus
- golden-signals
- etcd
- apiserver
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Observability Pillars 是什么
- 如何 Observability Pillars
trigger_keywords:
- Observability
- Pillars
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Observability Pillars

## Three Pillars

### Metrics

Numeric time-series data sampled at regular intervals.

**Golden Signals** (Google SRE methodology):
- **Latency**: Time to serve requests (p50, p95, p99)
- **Traffic**: Request rate per second
- **Errors**: Error rate (4xx, 5xx, failed health checks)
- **Saturation**: Resource utilization (CPU, memory, disk, network)

**Key Kubernetes metrics to monitor**:
- API Server: `apiserver_request_duration_seconds`, `apiserver_request_total`
- [[etcd|etcd]]: `etcd_disk_backend_commit_duration_seconds`, `etcd_mvcc_db_total_size_in_bytes`
- Scheduler: `scheduler_scheduling_attempt_duration_seconds`, `scheduler_pending_pods`
- [[kubelet|kubelet]]: `kubelet_pod_start_duration_seconds`, `kubelet_running_pods`

**Prometheus** is the de facto metrics collector, with Grafana for visualization.

### Logging

Structured and unstructured text output from containers and system components.

**Stacks**:
- **EFK**: Elasticsearch + Fluentd/Fluent Bit + Kibana
- **Loki**: Grafana Loki (label-based, Prometheus-style for logs)
- Centralized log aggregation with label-based querying

**Key log sources**: kube-apiserver, kubelet, controller-manager, scheduler, etcd, application containers.

### Distributed Tracing

Request-level trace across microservices.

**Components**:
- **Trace**: End-to-end request journey
- **Span**: Individual operation within a trace
- **Context Propagation**: Trace headers passed between services

**Tools**: Jaeger, Zipkin, OpenTelemetry (unified standard for traces, metrics, and logs).

## SLO/SLI Framework

- **SLI** (Service Level Indicator): What you measure (e.g., 99.9% of requests succeed)
- **SLO** (Service Level Objective): Target you aim for (e.g., 99.9% availability)
- **SLA** (Service Level Agreement): Contract with consequences for missing SLO
- **Error Budget**: 100% - SLO; the room for failure before breaching SLA

## 源码实现分析

### Prometheus 指标采集链路

```
┌─────────────────────────────────────────────────┐
│  kube-apiserver /metrics (HTTP)              │
│  kubelet /metrics (cAdvisor)                 │
│  etcd /metrics                               │
│  node-exporter :9100/metrics                 │
│  app /metrics (custom)                       │
└─────────────────┬───────────────────────────────┘
                  │ HTTP scrape (every 15-30s)
                  ▼
┌─────────────────────────────────────────────────┐
│  Prometheus Server                             │
│  ├── Service Discovery (K8s SD)              │
│  ├── TSDB (time-series storage)              │
│  ├── PromQL (query engine)                   │
│  └── Alertmanager (alert routing)            │
└─────────────────┬───────────────────────────────┘
                  │ PromQL / Grafana DataSource
                  ▼
┌─────────────────────────────────────────────────┐
│  Grafana (可视化) + Alertmanager (告警)      │
└─────────────────────────────────────────────────┘
```

### OpenTelemetry 统一采集模型

```go
// OTel SDK 统一 Traces + Metrics + Logs
// 应用代码:
tracer := otel.Tracer("my-service")
ctx, span := tracer.Start(ctx, "process-order")
defer span.End()

// OTel Collector 配置:
// receivers:  [otlp, prometheus, jaeger]
// processors: [batch, memory_limiter]
// exporters:  [prometheus, jaeger, loki]
// 统一收集 → 统一处理 → 多后端导出
```

## 使用场景

### 场景一：K8s 集群关键指标监控

```yaml
# PrometheusRule: 控制平面告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: control-plane-alerts
spec:
  groups:
  - name: apiserver
    rules:
    - alert: APIServerHighLatency
      expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket{verb!="WATCH"}[5m])) > 1
      for: 5m
      labels:
        severity: warning
    - alert: EtcdHighCommitLatency
      expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.25
      for: 5m
      labels:
        severity: critical
```

### 场景二：分布式追踪查询

```bash
# 🟢 低风险 - 通过 Jaeger UI 查询慢请求
# 访问 http://jaeger.internal:16686
# Service: order-service
# Operation: POST /api/orders
# Min Duration: 2s
# Tags: http.status_code=500

# 🟢 低风险 - 通过 OTel CLI 查询
kubectl -n observability port-forward svc/jaeger-query 16686:16686
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 有监控就有可观测性 | 监控是预定义图表，可观测性是能探索未知问题（需三支柱结合） |
| 日志越多越好 | 结构化日志 + 合理级别，过多日志增加成本和噪声 |
| Metrics 可以替代 Tracing | Metrics 看聚合趋势，Tracing 看单请求链路，互补不可替代 |
| SLO 就是 100% 可用 | SLO 应平衡可靠性与迭代速度，Error Budget 允许合理失败 |
| Prometheus 可以存储日志 | Prometheus 是时序指标数据库，日志应用 Loki/ES |
| 告警越多越安全 | 告警疲劳导致忽略真正问题，应只告警可操作的事件 |

## 面试要点

1. **可观测性三支柱如何协同？** — Metrics 发现问题（“什么异常”）→ Tracing 定位问题（“哪个环节”）→ Logs 确认根因（“具体错误”）。三者通过 trace_id 关联，实现从告警到根因的完整链路。

2. **Prometheus 在 K8s 中如何工作？** — Service Discovery 自动发现 Pod（通过 annotations）；每 15-30s HTTP 拉取 /metrics；TSDB 存储时序数据；PromQL 查询；Alertmanager 路由告警。Operator 模式管理（ServiceMonitor/PrometheusRule CRD）。

3. **SLO 如何制定和运营？** — SLI 选择（可用性/延迟/吐量）→ SLO 目标（99.9%）→ Error Budget（0.1%）→ 告警（多窗口多燃烧率）→ 复盘（Error Budget 耗尽时冻结发布）。工具：Sloth、OpenSLO。

4. **OpenTelemetry 的价值？** — 统一 Traces/Metrics/Logs 的采集标准，避免厂商锁定；SDK 自动埋点（HTTP/gRPC/DB）；Collector 统一收集→处理→导出多后端；与 Prometheus/Jaeger/Loki 无缝集成。

## Related
- [[概念/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合
- [[概念/控制器模式 × 可观测性.md|控制器模式 × 可观测性]] — 综合

- [[opentelemetry]] — OpenTelemetry
- [[fluentd]] — Fluentd
- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[技能/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[技能/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[概念/high-availability-patterns.md|High Availability Patterns]]

- [[概念/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]
- [[实体/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference


<!-- risk-assessed -->
