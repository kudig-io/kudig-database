---
title: "应用可观测性模式"
description: "生产级可观测性体系：结构化日志、分布式追踪、RED/USE 指标、SLO 定义与告警路由设计"
summary: "覆盖 Kubernetes 应用可观测性三大支柱（Logs/Metrics/Traces）的生产实践，包括结构化日志规范、OpenTelemetry 集成、RED/USE 指标体系、SLO/SLI 定义方法、告警分级路由和 Dashboard 设计原则。"
category: 应用模式
tags:
- patterns
- observability
- logging
- tracing
- metrics
- slo
- alerting
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 应用开发者
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- "K8s 应用可观测性体系怎么建设"
- "如何定义 SLO 和告警路由"
- "分布式追踪 OpenTelemetry 如何集成"
trigger_keywords:
- 可观测性
- Observability
- SLO
- 分布式追踪
- OpenTelemetry
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

# 应用可观测性模式

> **适用范围**: Kubernetes v1.28–v1.32 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

## 概述

可观测性（Observability）是生产系统运维的基石。没有可观测性的系统如同黑盒——故障发生时只能靠猜测定位。在 Kubernetes 环境中，Pod 的短暂性、服务的动态发现、多副本的并行处理，使得传统的"登录服务器看日志"方式完全失效。生产级可观测性需要系统性地建设三大支柱：结构化日志（Logs）、指标（Metrics）、分布式追踪（Traces），并在此之上构建 SLO 驱动的告警体系和高效的 Dashboard。

本文从应用开发者视角出发，提供可落地的可观测性工程实践。相关内容可参见 [[app-resilience-circuit-breaker]]、[[release-change-management-patterns]]、[[batch-cron-job-patterns]]。

---

## 模式定义与适用场景

### 可观测性三大支柱对比

| 支柱 | 回答的问题 | 数据特征 | 存储成本 | 典型工具 |
|------|-----------|---------|---------|---------|
| **Logs** | 发生了什么？为什么？ | 事件序列，高基数 | 高（按量） | Loki, Elasticsearch |
| **Metrics** | 系统状态如何？趋势？ | 时间序列，低基数 | 中（固定） | Prometheus, VictoriaMetrics |
| **Traces** | 请求经过了哪些路径？ | 因果链，中基数 | 中高（采样） | Jaeger, Tempo |

### 适用场景矩阵

| 场景 | 首选支柱 | 辅助支柱 | 说明 |
|------|---------|---------|------|
| 延迟突增定位 | Traces | Metrics | 追踪找到慢 span，指标确认范围 |
| 错误率上升 | Metrics | Logs | RED 指标告警，日志查具体错误 |
| 资源瓶颈 | Metrics (USE) | — | CPU/Mem/Disk/Net 利用率 |
| 业务逻辑错误 | Logs | Traces | 结构化日志记录业务上下文 |
| 跨服务调用链 | Traces | Logs | 全链路追踪 + 关联日志 |
| 容量规划 | Metrics | — | 长期趋势分析 |

---

## 架构设计

### 可观测性数据流架构

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Pod)                            │
│  ┌──────────┐  ┌──────────────┐  ┌───────────────────┐     │
│  │ 结构化日志 │  │ Metrics 端点  │  │ OTel SDK (Traces) │     │
│  │ stdout/err│  │ /metrics     │  │ OTLP exporter     │     │
│  └─────┬────┘  └──────┬───────┘  └────────┬──────────┘     │
├────────┼───────────────┼───────────────────┼────────────────┤
│        ▼               ▼                   ▼                │
│  ┌──────────┐  ┌──────────────┐  ┌───────────────────┐     │
│  │Promtail/ │  │ Prometheus   │  │ OTel Collector    │     │
│  │Fluent Bit│  │ (scrape)     │  │ (DaemonSet/Sidecar)│    │
│  └─────┬────┘  └──────┬───────┘  └────────┬──────────┘     │
│        ▼               ▼                   ▼                │
│  ┌──────────┐  ┌──────────────┐  ┌───────────────────┐     │
│  │  Loki /  │  │ Victoria     │  │  Jaeger / Tempo   │     │
│  │  ES      │  │ Metrics      │  │                   │     │
│  └──────────┘  └──────────────┘  └───────────────────┘     │
│        │               │                   │                │
│        └───────────────┼───────────────────┘                │
│                        ▼                                    │
│              ┌──────────────────┐                           │
│              │    Grafana       │                           │
│              │ (统一查询/告警)   │                           │
│              └──────────────────┘                           │
└─────────────────────────────────────────────────────────────┘
```

### 关键设计决策

1. **日志输出到 stdout**：容器化环境下，日志必须输出到 stdout/stderr，由 DaemonSet 采集器统一收集
2. **OTel Collector 部署模式**：DaemonSet（低开销）vs Sidecar（强隔离）vs Gateway（集中处理）
3. **Trace 采样策略**：Head-based（固定比例）vs Tail-based（基于结果），生产推荐 Tail-based 保留所有错误
4. **指标基数控制**：Label 维度不超过 5 个，避免高基数导致 Prometheus 内存爆炸

---

## K8s 实现

### 结构化日志规范

应用日志必须遵循 JSON 结构化格式，包含以下必选字段：

```json
{
  "timestamp": "2026-07-19T02:30:15.123Z",
  "level": "error",
  "logger": "com.example.order.OrderService",
  "message": "Failed to process payment",
  "trace_id": "abc123def456",
  "span_id": "789ghi",
  "service_name": "order-service",
  "service_version": "v2.3.1",
  "environment": "production",
  "request_id": "req-uuid-xxx",
  "duration_ms": 1523,
  "error_code": "PAYMENT_TIMEOUT",
  "context": {
    "order_id": "ORD-12345",
    "payment_provider": "stripe",
    "retry_count": 2
  }
}
```

### OpenTelemetry Collector DaemonSet 部署

```yaml
# 🟡 中风险：DaemonSet 在每个节点运行，配置不当可能影响节点资源
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: otel-collector
  namespace: observability
  labels:
    app.kubernetes.io/name: otel-collector
    app.kubernetes.io/component: agent
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: otel-collector
  template:
    metadata:
      labels:
        app.kubernetes.io/name: otel-collector
    spec:
      serviceAccountName: otel-collector
      containers:
        - name: collector
          image: otel/opentelemetry-collector-contrib:0.96.0
          args: ["--config=/etc/otel/config.yaml"]
          resources:
            requests:
              cpu: "200m"
              memory: "256Mi"
            limits:
              cpu: "500m"
              memory: "512Mi"
          ports:
            - containerPort: 4317  # OTLP gRPC
              name: otlp-grpc
            - containerPort: 4318  # OTLP HTTP
              name: otlp-http
          env:
            - name: NODE_NAME
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
            - name: POD_IP
              valueFrom:
                fieldRef:
                  fieldPath: status.podIP
          volumeMounts:
            - name: config
              mountPath: /etc/otel
            - name: varlog
              mountPath: /var/log
              readOnly: true
          livenessProbe:
            httpGet:
              path: /
              port: 13133
            initialDelaySeconds: 10
          readinessProbe:
            httpGet:
              path: /
              port: 13133
      volumes:
        - name: config
          configMap:
            name: otel-collector-config
        - name: varlog
          hostPath:
            path: /var/log
```

### ServiceMonitor 自动发现

```yaml
# 🟢 低风险：声明式配置，Prometheus Operator 自动处理
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: order-service-monitor
  namespace: monitoring
  labels:
    release: prometheus-stack
spec:
  namespaceSelector:
    matchNames:
      - production
  selector:
    matchLabels:
      app.kubernetes.io/name: order-service
  endpoints:
    - port: http-metrics
      path: /metrics
      interval: 30s
      scrapeTimeout: 10s
      # Relabel：添加集群和环境标签
      relabelings:
        - sourceLabels: [__meta_kubernetes_namespace]
          targetLabel: namespace
        - sourceLabels: [__meta_kubernetes_pod_name]
          targetLabel: pod
      # 指标过滤：只保留关键指标，降低存储成本
      metricRelabelings:
        - sourceLabels: [__name__]
          regex: "(http_request_duration_seconds.*|http_requests_total|process_.*)"
          action: keep
```

### Pod 可观测性注解

```yaml
# 🟢 低风险：Pod 模板中的可观测性配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
spec:
  template:
    metadata:
      labels:
        app.kubernetes.io/name: order-service
      annotations:
        # Prometheus 自动发现
        prometheus.io/scrape: "true"
        prometheus.io/port: "9090"
        prometheus.io/path: "/metrics"
        # 日志采集配置
        fluentbit.io/parser: json
        fluentbit.io/exclude: "false"
    spec:
      containers:
        - name: app
          image: registry.internal/order-service:v2.3.1
          ports:
            - name: http
              containerPort: 8080
            - name: http-metrics
              containerPort: 9090
          env:
            # OpenTelemetry 环境变量配置
            - name: OTEL_SERVICE_NAME
              value: "order-service"
            - name: OTEL_EXPORTER_OTLP_ENDPOINT
              value: "http://otel-collector.observability.svc:4317"
            - name: OTEL_TRACES_SAMPLER
              value: "parentbased_traceidratio"
            - name: OTEL_TRACES_SAMPLER_ARG
              value: "0.1"  # 10% 采样率
            - name: OTEL_RESOURCE_ATTRIBUTES
              value: "service.version=v2.3.1,deployment.environment=production"
            # 日志级别动态控制
            - name: LOG_LEVEL
              value: "info"
```

---

## 生产配置示例

### RED 指标体系实现

RED（Rate, Errors, Duration）是面向请求型服务的黄金指标：

```
# Rate：请求速率
sum(rate(http_requests_total{service="order-service"}[5m])) by (endpoint, method)

# Errors：错误率
sum(rate(http_requests_total{service="order-service", status=~"5.."}[5m]))
/
sum(rate(http_requests_total{service="order-service"}[5m]))

# Duration：延迟分布（P50/P95/P99）
histogram_quantile(0.99,
  sum(rate(http_request_duration_seconds_bucket{service="order-service"}[5m])) by (le, endpoint)
)
```

### USE 指标体系（资源维度）

USE（Utilization, Saturation, Errors）面向基础设施：

```
# Utilization：CPU 利用率
1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)

# Saturation：内存压力
node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes < 0.1

# Errors：网络错误
rate(node_network_receive_errs_total[5m]) + rate(node_network_transmit_errs_total[5m])
```

### SLO 定义与 Error Budget

```yaml
# 🟢 低风险：PrometheusRule 声明 SLO 告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: order-service-slo
  namespace: monitoring
spec:
  groups:
    - name: order-service.slo
      rules:
        # SLI：可用性 = 成功请求 / 总请求
        - record: sli:order_service:availability
          expr: |
            sum(rate(http_requests_total{service="order-service", status!~"5.."}[30d]))
            /
            sum(rate(http_requests_total{service="order-service"}[30d]))

        # SLO 目标：99.9% 可用性
        # Error Budget 消耗速率告警（多窗口多燃烧率）
        - alert: OrderServiceHighErrorBurnRate
          expr: |
            (
              sum(rate(http_requests_total{service="order-service", status=~"5.."}[1h]))
              /
              sum(rate(http_requests_total{service="order-service"}[1h]))
            ) > (14.4 * 0.001)
            and
            (
              sum(rate(http_requests_total{service="order-service", status=~"5.."}[5m]))
              /
              sum(rate(http_requests_total{service="order-service"}[5m]))
            ) > (14.4 * 0.001)
          for: 2m
          labels:
            severity: critical
            slo: order-service-availability
          annotations:
            summary: "Order Service Error Budget 快速消耗（1h 窗口 > 14.4x）"
            description: "当前错误率将在 2 天内耗尽 30 天 Error Budget"
            runbook: "https://wiki.internal/runbooks/order-service-slo-breach"

        # 慢燃烧率告警
        - alert: OrderServiceSlowErrorBurnRate
          expr: |
            (
              sum(rate(http_requests_total{service="order-service", status=~"5.."}[6h]))
              /
              sum(rate(http_requests_total{service="order-service"}[6h]))
            ) > (3 * 0.001)
          for: 15m
          labels:
            severity: warning
            slo: order-service-availability
          annotations:
            summary: "Order Service Error Budget 缓慢消耗（6h 窗口 > 3x）"
```

### 告警路由配置

```yaml
# 🟡 中风险：修改告警路由影响通知链路
apiVersion: v1
kind: ConfigMap
metadata:
  name: alertmanager-config
  namespace: monitoring
data:
  alertmanager.yaml: |
    route:
      receiver: default-team
      group_by: [alertname, namespace, severity]
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 4h
      routes:
        # P0：SLO 燃烧率告警 → 即时电话
        - match:
            severity: critical
            slo: ".*"
          receiver: sre-oncall-phone
          repeat_interval: 15m
          continue: true

        # P1：服务级告警 → Slack + 邮件
        - match:
            severity: critical
          receiver: team-slack-critical
          repeat_interval: 1h

        # P2：警告级 → 仅 Slack
        - match:
            severity: warning
          receiver: team-slack-warning
          repeat_interval: 4h

        # 批处理任务告警 → 数据团队
        - match:
            team: data-engineering
          receiver: data-team-slack

    receivers:
      - name: sre-oncall-phone
        pagerduty_configs:
          - service_key: "<pagerduty-key>"
            severity: critical
      - name: team-slack-critical
        slack_configs:
          - channel: "#alerts-critical"
            send_resolved: true
      - name: team-slack-warning
        slack_configs:
          - channel: "#alerts-warning"
            send_resolved: false
      - name: data-team-slack
        slack_configs:
          - channel: "#data-alerts"
      - name: default-team
        slack_configs:
          - channel: "#alerts-default"
```

---

## 运维要点

### Dashboard 设计原则

生产级 Grafana Dashboard 应遵循分层设计：

| 层级 | 受众 | 内容 | 刷新频率 |
|------|------|------|---------|
| L1 概览 | 管理层/SRE Lead | SLO 状态、Error Budget、服务健康红绿灯 | 5min |
| L2 服务 | 服务 Owner | RED 指标、依赖健康、资源使用 | 1min |
| L3 调试 | 开发者 | 单 Pod 指标、日志流、Trace 详情 | 实时 |
| L4 基础设施 | 平台团队 | 节点 USE、网络、存储、控制平面 | 1min |

### 日志查询最佳实践

```bash
# 🟢 低风险：通过 Loki 查询特定 trace 的所有日志
# LogQL: 通过 trace_id 关联所有服务日志
{namespace="production"} |= "abc123def456" | json | level="error"

# 🟢 低风险：查看特定服务最近 5 分钟的错误日志
kubectl logs -n production -l app.kubernetes.io/name=order-service \
  --since=5m --tail=100 | jq 'select(.level == "error")'

# 🟢 低风险：查看 Pod 重启原因
kubectl get events -n production --field-selector reason=BackOff \
  --sort-by='.lastTimestamp'
```

### Trace 采样策略建议

| 环境 | 采样率 | 策略 | 说明 |
|------|--------|------|------|
| 开发/测试 | 100% | AlwaysOn | 全量采集用于调试 |
| 预发布 | 50% | TraceIdRatio | 高覆盖验证 |
| 生产（常规） | 10% | ParentBased + Ratio | 平衡成本与覆盖 |
| 生产（错误） | 100% | Tail-based (error) | 所有错误请求必须保留 |
| 生产（慢请求） | 100% | Tail-based (latency > P99) | 慢请求必须保留 |

### 指标基数控制

```bash
# 🟢 低风险：检查 Prometheus 指标基数
# 查看 top 10 高基数指标
curl -s 'http://prometheus:9090/api/v1/label/__name__/values' | \
  jq -r '.data[]' | head -20

# 🟢 低风险：查看特定指标的 series 数量
curl -s 'http://prometheus:9090/api/v1/query?query=count(http_requests_total)'

# 🟡 中风险：强制删除过期 series（释放内存）
curl -X POST 'http://prometheus:9090/api/v1/admin/tsdb/delete_series?match[]={job="old-job"}'
```

---

## 反模式

### 反模式 1：日志中包含敏感信息

```json
// ❌ 错误：日志中记录完整信用卡号、密码
{"message": "User login", "password": "xxx", "card": "4111-1111-1111-1111"}
```

**后果**：日志系统成为数据泄露源，违反 PCI-DSS/GDPR 合规要求。

**修正**：敏感字段脱敏处理，使用 `***` 或仅保留后 4 位。在 OTel Collector 中配置 `redaction` processor。

### 反模式 2：Metrics Label 高基数爆炸

```
// ❌ 错误：将 user_id、request_id 作为 label
http_requests_total{user_id="12345", request_id="uuid-xxx", ...}
```

**后果**：Prometheus 内存爆炸，查询超时，最终 OOMKill。每个唯一 label 组合创建一个 time series。

**修正**：高基数数据放 Logs/Traces，Metrics 只保留有限枚举值（endpoint, method, status_code）。

### 反模式 3：告警风暴无分级

**后果**：所有告警发到同一频道，on-call 工程师告警疲劳，真正的 P0 被淹没。

**修正**：多窗口多燃烧率告警 + 分级路由 + 抑制规则。参见 [[release-change-management-patterns]]。

### 反模式 4：只有 Metrics 没有 Traces

**后果**：知道"慢了"但不知道"慢在哪里"，跨服务调用链问题无法定位。

**修正**：至少对入口服务和关键路径集成 OTel Tracing，错误和慢请求 100% 采样。

### 反模式 5：Dashboard 堆砌无层次

**后果**：一个 Dashboard 50 个 Panel，故障时找不到关键信息。

**修正**：L1→L2→L3 分层，每层不超过 12 个 Panel，L1 只放 SLO 和红绿灯。

---

## Related

- [[app-resilience-circuit-breaker]] — 应用弹性与熔断模式
- [[release-change-management-patterns]] — 发布变更管理模式
- [[batch-cron-job-patterns]] — 批处理与定时任务模式
- [[application-runbooks]] — 应用运维 Runbook
- [[pod-availability-lifecycle]] — Pod 可用性与生命周期管理
- [[ai-inference-app-patterns]] — AI 推理应用模式
