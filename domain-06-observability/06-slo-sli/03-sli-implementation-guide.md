---
title: SLI 实现指南：可用性、延迟、吞吐量
description: 面向阿里云/专有云 K8s 的 SLI 实现指南，讲解可用性、延迟、吞吐量三类核心 SLI 的采集、计算与告警实现。
category: observability
tags:
- k8s
- sli
- availability
- latency
- throughput
- prometheus
- observability
- sre
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 20min
intent_queries:
- SLI 实现指南
- 可用性延迟吞吐量 SLI
- K8s Prometheus SLI 采集
trigger_keywords:
- SLI
- availability
- latency
- throughput
- 可用性
- 延迟
- 吞吐量
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- slo-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-06-29"
updated: "2026-06-29"
---

# SLI 实现指南：可用性、延迟、吞吐量

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，详细讲解可用性、延迟、吞吐量三类 SLI 的 Prometheus 实现方法。

## 目录

1. [SLI 实现总览](#sli-实现总览)
2. [可用性 SLI](#可用性-sli)
3. [延迟 SLI](#延迟-sli)
4. [吞吐量 SLI](#吞吐量-sli)
5. [统一 SLI 导出](#统一-sli-导出)
6. [Dashboard 与 Alert](#dashboard-与-alert)
7. [常见问题](#常见问题)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. SLI 实现总览

### 1.1 三类核心 SLI

| SLI | 定义 | 常用指标 | 测量位置 |
|:---|:---|:---|:---|
| 可用性 | 服务可成功响应请求的比例 | HTTP 2xx/3xx / 总请求 | Ingress / LB |
| 延迟 | 请求处理时间分布 | P50/P95/P99 | Ingress / Service |
| 吞吐量 | 单位时间处理的请求量 | RPS/QPS | Ingress / Service |

### 1.2 数据采集方式

| 方式 | 优点 | 缺点 |
|:---|:---|:---|
| Ingress 侧 | 覆盖所有入口流量 | 无法感知服务内部错误 |
| Sidecar/Agent | 应用无侵入 | 增加资源开销 |
| 应用内埋点 | 最准确 | 需要改造应用 |
| 客户端探测 | 模拟真实用户 | 覆盖有限 |

---

## 2. 可用性 SLI

### 2.1 基于 Ingress 的可用性

```yaml
# Prometheus 记录规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sli-availability
  namespace: monitoring
spec:
  groups:
    - name: sli.availability
      interval: 60s
      rules:
        - record: sli:availability:ratio_30d
          expr: |
            1 - (
              sum(rate(nginx_ingress_controller_requests{status=~"5.."}[30d]))
              /
              sum(rate(nginx_ingress_controller_requests[30d]))
            )
          labels:
            slo: "availability"
```

### 2.2 基于应用指标的可用性

```yaml
        - record: sli:availability:ratio_30d
          expr: |
            1 - (
              sum(rate(http_requests_total{service="order-service",status=~"5.."}[30d]))
              /
              sum(rate(http_requests_total{service="order-service"}[30d]))
            )
```

### 2.3 可用性告警

```yaml
        - alert: AvailabilitySLOViolation
          expr: sli:availability:ratio_30d < 0.999
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "可用性 SLO 未达成"
```

---

## 3. 延迟 SLI

### 3.1 直方图指标

应用需暴露 histogram 指标：

```python
# Python Flask 示例
from prometheus_client import Histogram
request_duration = Histogram('http_request_duration_seconds', 'Request duration', ['method', 'endpoint'])
```

### 3.2 Prometheus 记录规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sli-latency
  namespace: monitoring
spec:
  groups:
    - name: sli.latency
      interval: 60s
      rules:
        - record: sli:latency_p95:seconds_30d
          expr: |
            histogram_quantile(0.95,
              sum(rate(http_request_duration_seconds_bucket{service="order-service"}[30d])) by (le)
            )
        - record: sli:latency_good:ratio_30d
          expr: |
            sum(rate(http_request_duration_seconds_bucket{service="order-service",le="0.2"}[30d]))
            /
            sum(rate(http_request_duration_seconds_count{service="order-service"}[30d]))
```

### 3.3 延迟告警

```yaml
        - alert: LatencySLOViolation
          expr: sli:latency_good:ratio_30d < 0.99
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "延迟 SLO 未达成"
```

---

## 4. 吞吐量 SLI

### 4.1 吞吐量计算

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sli-throughput
  namespace: monitoring
spec:
  groups:
    - name: sli.throughput
      interval: 60s
      rules:
        - record: sli:throughput:rps_5m
          expr: |
            sum(rate(http_requests_total{service="order-service"}[5m]))
        - record: sli:throughput:peak_ratio_30d
          expr: |
            sli:throughput:rps_5m / max_over_time(sli:throughput:rps_5m[30d])
```

### 4.2 吞吐量告警

```yaml
        - alert: ThroughputDrop
          expr: |
            sli:throughput:rps_5m
            /
            avg_over_time(sli:throughput:rps_5m[1h] offset 1h) < 0.5
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "吞吐量较上小时下降超过 50%"
```

---

## 5. 统一 SLI 导出

### 5.1 OpenSLO 规范

```yaml
apiVersion: openslo/v1
kind: SLO
metadata:
  name: order-service-availability
spec:
  service: order-service
  description: "订单服务可用性 SLO"
  budgetingMethod: Occurrences
  objectives:
    - ratio: 0.999
      displayName: "可用性"
  indicator:
    metadata:
      name: order-service-availability
    spec:
      ratioMetric:
        counter: true
        good:
          metricSource:
            type: Prometheus
            metricSourceRef: prometheus
            query: sum(rate(http_requests_total{service="order-service",status!~"5.."}[{{.Window}}]))
        total:
          metricSource:
            type: Prometheus
            metricSourceRef: prometheus
            query: sum(rate(http_requests_total{service="order-service"}[{{.Window}}]))
```

---

## 6. Dashboard 与 Alert

### 6.1 Grafana 仪表盘关键面板

| 面板 | 指标 |
|:---|:---|
| 可用性趋势 | `sli:availability:ratio_30d` |
| P95 延迟 | `sli:latency_p95:seconds_30d` |
| 吞吐量 | `sli:throughput:rps_5m` |
| 错误预算剩余 | 自定义计算 |
| Burn Rate | `sli:burn_rate:5m` |

---

## 7. 常见问题

| 问题 | 原因 | 解决 |
|:---|:---|:---|
| SLI 数值波动大 | 时间窗口太短 | 使用 30d 滚动窗口 |
| 缺少 histogram 数据 | 应用未暴露 | 添加 Prometheus client 库 |
| Ingress 指标不包含内部调用 | 测量点不对 | 结合 Service Mesh 指标 |
| 告警误报多 | 阈值过严 | 调整 SLO 目标值 |

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 可用性指标 | 基于入口或应用指标 | PrometheusRule |
| 延迟直方图 | 暴露 bucket 指标 | 应用端点 |
| 吞吐量指标 | 实时 RPS | PrometheusRule |
| SLI 记录规则 | 统一命名 | `kubectl get prometheusrules` |
| Dashboard | 可视化展示 | Grafana |
| 告警规则 | SLO 违规触发 | Alertmanager |

---

## SLI 数据质量保障

SLI 的准确性直接影响 SLO 可信度。需要从采集、计算、存储三个环节保障数据质量。

| 环节 | 风险 | 保障措施 |
|:---|:---|:---|
| 采集 | 指标缺失或标签错误 | 统一埋点规范、验证 label |
| 计算 | PromQL 错误、bucket 不当 | 代码审查、与人工计算对比 |
| 存储 | 数据丢失、采样 | 高可用 Prometheus、长期存储 |

### SLI 与 Tracing 结合

分布式追踪可以帮助定位 SLI 恶化的具体链路。

```bash
# 使用 Jaeger 查询高延迟 trace
kubectl port-forward svc/jaeger-query 16686:16686 -n observability
```

在 Jaeger UI 中按 `service=order-service` 与 `duration>500ms` 过滤，分析慢请求经过的每个 span。

### SLI 异常排查

当 SLI 突然恶化时：

1. 确认数据源是否正常（Prometheus、日志、探针）
2. 检查是否因发布、变更、依赖故障导致
3. 通过下钻到具体实例、Pod、节点定位根因
4. 修复后重新计算 SLI，观察是否恢复

## SLI 与用户体验对齐

SLI 的最终目标是反映用户体验。技术团队容易陷入采集容易实现的指标，而忽略用户真正关心的方面。

### 用户视角 vs 系统视角

| 用户视角 | 系统视角 | 建议 |
|:---|:---|:---|
| 页面打不开 | API 返回 500 | 同时监控入口可用性 |
| 操作卡顿 | P99 延迟高 | 按用户操作路径聚合延迟 |
| 数据不一致 | 数据库同步延迟 | 增加正确性 SLI |
| 消息收不到 | 消息队列积压 | 增加端到端延迟 SLI |

### SLI 告警分层

| 层级 | 目的 | 示例 |
|:---|:---|:---|
| 用户体验层 | 快速发现业务影响 | 入口 5xx、P99 延迟 |
| 服务层 | 定位到具体服务 | 各微服务错误率 |
| 基础设施层 | 发现资源或底座问题 | 节点 CPU、网络丢包 |

## 典型工单场景与处理

**场景**：用户投诉页面慢，但监控显示 API 延迟正常。

处理步骤：
1. 检查前端监控（FCP/LCP/CLS）是否异常。
2. 分析静态资源、CDN、DNS 耗时。
3. 比较服务端 P99 与前端 P99，定位瓶颈环节。
4. 补充缺失的 SLI，如下载速度、首屏时间。

## SLI 实现工具链

| 工具 | 用途 | 适用 SLI |
|:---|:---|:---|
| Prometheus | 指标采集与计算 | 可用性、延迟、吞吐 |
| Grafana | 可视化 | 所有 SLI |
| Blackbox Exporter | 端点探测 | 可用性 |
| Jaeger / Tempo | 分布式追踪 | 延迟、错误链路 |
| Loki | 日志聚合 | 错误分类、业务指标 |
| OpenTelemetry | 统一埋点 | 全链路指标与追踪 |

### SLI 命名规范

统一的 SLI 命名有助于查询与治理：

```
sli:<service>:<metric>:<aggregation>
```

示例：

- `sli:order-service:availability:30d`
- `sli:order-service:latency_p95:30d`

### SLI 异常排查流程

1. 确认 SLI 数据源正常。
2. 查看相同时间段的相关变更。
3. 下钻到实例、Pod、节点维度。
4. 结合日志与追踪定位根因。
5. 修复后重新计算 SLI，确认恢复。

## SLI 实施检查清单

- [ ] 已明确 SLI 所代表的用户体验维度
- [ ] 已确定数据源与采集方式
- [ ] PromQL / 日志查询已验证可正确计算 SLI
- [ ] 已配置 Grafana 面板展示 SLI 与 SLO 差距
- [ ] 已设置 burn rate 告警
- [ ] 已记录 SLI 负责人与 review 周期
- [ ] 已与业务方确认 SLI 与 SLO 的合理性

### SLI 与告警结合

SLI 应驱动告警，而非仅用于展示。例如：

```promql
# 可用性 SLI 低于目标时触发
(
  sum(rate(http_requests_total{status!~"5.."}[5m]))
  /
  sum(rate(http_requests_total[5m]))
) < 0.999
```

## SLI 与 Prometheus Recording Rule

对于复杂 SLI，建议使用 Recording Rule 预计算，提高查询效率并保证一致性。

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sli-recording-rules
  namespace: monitoring
spec:
  groups:
    - name: sli
      interval: 60s
      rules:
        - record: sli:order_service:availability:rate5m
          expr: |
            sum(rate(http_requests_total{service="order-service",status!~"5.."}[5m]))
            /
            sum(rate(http_requests_total{service="order-service"}[5m]))
        - record: sli:order_service:latency_p95:rate5m
          expr: |
            histogram_quantile(0.95,
              sum(rate(http_request_duration_seconds_bucket{service="order-service"}[5m])) by (le)
            )
```

### SLI 与 Grafana 面板

建议每个服务配置一个 SLI 面板，包含：

- 当前 SLI 数值与 SLO 目标线
- 30 天趋势图
- 错误预算剩余量
- 最近事件标注

## Related

- [[domain-06-observability/06-slo-sli/18-slo-sli-system.md|SLO/SLI体系建设与管理]]
- [[domain-06-observability/06-slo-sli/01-slo-engineering-practice.md|SLO 工程实践]]

## See Also

- [[domain-06-observability/06-slo-sli/02-error-budget-policy.md|错误预算政策与 burn rate alert]]
- [[domain-06-observability/02-metrics/01-prometheus-enterprise-monitoring.md|Prometheus 企业监控]]
