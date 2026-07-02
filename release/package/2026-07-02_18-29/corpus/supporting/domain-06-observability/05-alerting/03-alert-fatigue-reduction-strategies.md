---
title: 告警疲劳治理策略
description: '告警疲劳治理：告警合并与去重、SLO-based alerting (多窗口多 burn rate)、动态阈值 (ML-based)、告警优先级分级、定期告警审查'
summary: '告警合并、SLO 告警、动态阈值与告警优先级分级'
category: observability
tags:
- alert-fatigue
- slo-alerting
- burn-rate
- dynamic-threshold
- alert-management
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
- 告警疲劳治理策略是什么
- 如何减少告警疲劳
trigger_keywords:
- 告警疲劳
- Alert Fatigue
- SLO Alerting
- Burn Rate
- 动态阈值
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


# 告警疲劳治理策略

## 概述

告警疲劳（Alert Fatigue）是 SRE 面临的核心挑战之一。过多的噪音告警导致工程师对关键告警反应迟钝。本文档提供系统化的告警疲劳治理方案，包括告警合并、SLO-based alerting、动态阈值和告警审查机制。

## 1. 告警疲劳根因分析

### 1.1 常见问题

| 问题 | 表现 | 影响 |
|------|------|------|
| 阈值设置不当 | 频繁触发、快速恢复 | 噪音过多 |
| 缺少分组 | 同一问题多个告警 | 告警风暴 |
| 无抑制规则 | 级联故障产生大量告警 | 信息过载 |
| 告警粒度太细 | 每个 Pod 独立告警 | 难以定位根因 |
| 缺少 SLO 视角 | 技术指标 vs 业务影响 | 优先级混乱 |

## 2. 告警合并与去重

### 2.1 分组策略优化

```yaml
# Alertmanager 分组配置
route:
  # 按 alertname + namespace 分组
  group_by: ['alertname', 'namespace']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h

  routes:
  # 紧急告警：更细分组
  - match:
      severity: critical
    group_by: ['alertname', 'namespace', 'pod']
    group_wait: 10s
    group_interval: 1m

  # 节点告警：按节点分组
  - match:
      category: node
    group_by: ['alertname', 'node']

  # 网络告警：按集群分组
  - match:
      category: network
    group_by: ['alertname', 'cluster']
```

### 2.2 抑制规则

```yaml
inhibit_rules:
# 节点 Down 抑制该节点上所有 Pod 告警
- source_match:
    alertname: NodeDown
  target_match_re:
    alertname: .*
  equal: ['node']

# 集群级别告警抑制命名空间级别告警
- source_match:
    alertname: ClusterUnhealthy
  target_match_re:
    alertname: .*
  equal: ['cluster']

# 关键服务 Down 抑制性能告警
- source_match:
    alertname: ServiceDown
  target_match_re:
    alertname: (HighLatency|HighErrorRate|HighCPU)
  equal: ['namespace', 'service']
```

## 3. SLO-based Alerting

### 3.1 SLO 定义

```yaml
# SLO 定义示例
# 服务：payment-service
# SLI：成功请求率
# SLO：99.9% 的请求在 30 天窗口内成功
# Error Budget：0.1% = 43.2 分钟/月

apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: payment-service-slo
  namespace: production
spec:
  groups:
  - name: slo.sli
    interval: 1m
    rules:
    # SLI 指标：成功请求率
    - record: sli:payment_service:success_rate:5m
      expr: |
        sum(rate(http_requests_total{
          namespace="production",
          service="payment-service",
          code!~"5.."
        }[5m]))
        /
        sum(rate(http_requests_total{
          namespace="production",
          service="payment-service"
        }[5m]))

    # Error Budget 剩余比例
    - record: sli:payment_service:error_budget_remaining:30d
      expr: |
        1 - (
          (1 - avg_over_time(sli:payment_service:success_rate:5m[30d]))
          /
          (1 - 0.999)
        )
```

### 3.2 多窗口多 Burn Rate 告警

```yaml
# 基于 Google SRE Book 的多窗口多 Burn Rate 方案
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: payment-service-burn-rate
  namespace: production
spec:
  groups:
  - name: slo.burn-rate
    rules:
    # 2% budget consumed in 1 hour (fast burn)
    - alert: SLOBurnRateHigh
      expr: |
        sli:payment_service:success_rate:5m < (1 - (2 * (1 - 0.999) / 100))
        and
        sli:payment_service:success_rate:1h < (1 - (2 * (1 - 0.999) / 100))
      for: 2m
      labels:
        severity: critical
        slo: payment-service
      annotations:
        summary: "High burn rate for payment-service SLO"
        description: "Error budget burning at 2x rate, will exhaust in 15 days"

    # 5% budget consumed in 6 hours (medium burn)
    - alert: SLOBurnRateMedium
      expr: |
        sli:payment_service:success_rate:30m < (1 - (5 * (1 - 0.999) / 100))
        and
        sli:payment_service:success_rate:6h < (1 - (5 * (1 - 0.999) / 100))
      for: 15m
      labels:
        severity: warning
        slo: payment-service
      annotations:
        summary: "Medium burn rate for payment-service SLO"
        description: "Error budget burning at 5x rate, will exhaust in 6 days"

    # 10% budget consumed in 3 days (slow burn)
    - alert: SLOBurnRateSlow
      expr: |
        sli:payment_service:success_rate:2h < (1 - (10 * (1 - 0.999) / 100))
        and
        sli:payment_service:success_rate:3d < (1 - (10 * (1 - 0.999) / 100))
      for: 1h
      labels:
        severity: warning
        slo: payment-service
      annotations:
        summary: "Slow burn rate for payment-service SLO"
        description: "Error budget burning at 10x rate, will exhaust in 3 days"
```

### 3.3 Burn Rate 速查表

| Burn Rate | 预计耗尽时间 | 窗口大小 | 严重级别 | 适用场景 |
|-----------|-------------|---------|---------|---------|
| 14.4x | 2 天 | 1 小时 | critical | 快速燃烧，立即响应 |
| 6x | 5 天 | 6 小时 | warning | 中等燃烧，当日处理 |
| 3x | 10 天 | 1 天 | warning | 慢速燃烧，本周处理 |
| 1x | 30 天 | 3 天 | info | 正常燃烧，关注趋势 |

## 4. 动态阈值（ML-based）

### 4.1 基于历史数据的动态阈值

```yaml
# 使用 Prometheus 的 predict_linear 函数
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dynamic-thresholds
spec:
  groups:
  - name: dynamic.rules
    rules:
    # 基于历史趋势预测的 CPU 告警
    - alert: HighCPUPredicted
      expr: |
        predict_linear(
          container_cpu_usage_seconds_total{namespace="production"}[1h],
          3600
        ) > 0.8
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "CPU usage predicted to exceed 80% in 1 hour"

    # 基于历史模式的异常检测
    - alert: AnomalousLatency
      expr: |
        histogram_quantile(0.99,
          rate(http_request_duration_seconds_bucket{
            namespace="production"
          }[5m])
        )
        >
        avg_over_time(
          histogram_quantile(0.99,
            rate(http_request_duration_seconds_bucket{
              namespace="production"
            }[5m])
          )[7d:5m]
        ) + 3 * stddev_over_time(
          histogram_quantile(0.99,
            rate(http_request_duration_seconds_bucket{
              namespace="production"
            }[5m])
          )[7d:5m]
        )
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Latency anomaly detected"
        description: "P99 latency is 3 standard deviations above 7-day average"
```

### 4.2 周期性基线

```yaml
# 使用 Grafana Mimir 的异常检测
# 或外部 ML 服务（如 Datadog、New Relic）

# 示例：基于星期几和时间段的动态阈值
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: periodic-baseline
spec:
  groups:
  - name: baseline.rules
    rules:
    # 工作日 vs 周末的动态阈值
    - alert: TrafficAnomaly
      expr: |
        # 当前 QPS
        sum(rate(http_requests_total{namespace="production"}[5m]))
        >
        # 历史同时段 QPS（周同比）+ 2 倍标准差
        (
          avg_over_time(
            sum(rate(http_requests_total{namespace="production"}[5m]))[7d:5m]
          )
          + 2 * stddev_over_time(
            sum(rate(http_requests_total{namespace="production"}[5m]))[7d:5m]
          )
        ) * 1.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Traffic anomaly detected"
```

## 5. 告警优先级分级

### 5.1 优先级定义

| 优先级 | 定义 | 响应时间 | 通知方式 | 示例 |
|--------|------|---------|---------|------|
| P0 Critical | 服务完全不可用 | 5 分钟 | 电话+短信+IM | 全站宕机、数据丢失 |
| P1 High | 服务严重降级 | 15 分钟 | 短信+IM | API 延迟 >5s、错误率 >10% |
| P2 Medium | 服务部分影响 | 1 小时 | IM+邮件 | 单实例故障、容量预警 |
| P3 Low | 潜在风险 | 下一工作日 | 邮件 | 证书即将过期、资源使用率高 |

### 5.2 优先级路由配置

```yaml
route:
  receiver: slack-default
  routes:
  # P0 Critical
  - match:
      severity: critical
    receiver: pagerduty-p0
    group_wait: 10s
    repeat_interval: 5m
    routes:
    - match:
        priority: P0
      receiver: phone-call
      group_wait: 0s

  # P1 High
  - match:
      severity: warning
    receiver: pagerduty-p1
    group_wait: 30s
    repeat_interval: 30m

  # P2 Medium
  - match:
      severity: warning
      priority: P2
    receiver: slack-production
    group_wait: 5m
    repeat_interval: 4h

  # P3 Low
  - match:
      severity: info
    receiver: email-team
    group_wait: 15m
    repeat_interval: 24h
```

## 6. 定期告警审查

### 6.1 告警审查流程

```bash
#!/bin/bash
# 告警审查脚本
set -euo pipefail

echo "=== 告警审查报告 ==="
echo "审查时间: $(date)"
echo ""

# 1. 获取过去 7 天的告警统计
echo "1. 告警触发统计（过去 7 天）"
curl -s "http://prometheus:9090/api/v1/query" \
  --data-urlencode 'query=sum by (alertname) (increase(ALERTS{alertstate="firing"}[7d]))' | \
  jq -r '.data.result[] | "\(.metric.alertname): \(.value[1]) 次"' | \
  sort -t: -k2 -nr

echo ""
echo "2. Top 10 最频繁告警"
curl -s "http://prometheus:9090/api/v1/query" \
  --data-urlencode 'query=topk(10, sum by (alertname) (increase(ALERTS{alertstate="firing"}[7d])))' | \
  jq -r '.data.result[] | "\(.metric.alertname): \(.value[1]) 次"'

echo ""
echo "3. 告警恢复时间统计"
curl -s "http://prometheus:9090/api/v1/query" \
  --data-urlencode 'query=avg by (alertname) (ALERTS_FOR{alertstate="firing"})' | \
  jq -r '.data.result[] | "\(.metric.alertname): \(.)"'
```

### 6.2 告警健康度指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: alert-health-metrics
spec:
  groups:
  - name: alert.health
    rules:
    # 告警疲劳指数（每天触发次数）
    - record: alert:fatigue_index:daily
      expr: |
        sum by (alertname, namespace) (
          increase(ALERTS{alertstate="firing"}[1d])
        )

    # 误报率（快速恢复的告警比例）
    - record: alert:false_positive_rate:7d
      expr: |
        sum by (alertname) (
          increase(alertmanager_alerts_resolved_total[7d])
          and
          increase(alertmanager_alerts_resolved_total{resolved_reason="timeout"}[7d]) < 1
        )
        /
        sum by (alertname) (
          increase(alertmanager_alerts_received_total[7d])
        )

    # 告警响应 SLA
    - record: alert:response_sla:7d
      expr: |
        sum by (alertname) (
          increase(alertmanager_alerts_silenced_total[7d])
        )
        /
        sum by (alertname) (
          increase(alertmanager_alerts_received_total[7d])
        )

    # 疲劳指数告警
    - alert: AlertFatigueDetected
      expr: |
        alert:fatigue_index:daily > 50
      for: 1d
      labels:
        severity: warning
      annotations:
        summary: "High alert fatigue detected"
        description: "Alert {{ $labels.alertname }} triggered {{ $value }} times in 24h"
```

### 6.3 告警审查清单

```
告警审查检查清单（每月执行）：

□ 检查 Top 10 最频繁告警
  - 是否有告警触发 > 100 次/周？
  - 是否有告警恢复时间 < 1 分钟？

□ 检查误报率
  - 哪些告警的误报率 > 30%？
  - 是否需要调整阈值？

□ 检查告警覆盖
  - 是否有 SLO 没有对应的告警？
  - 是否有关键路径缺少告警？

□ 检查告警响应
  - 平均响应时间是否符合 SLA？
  - 是否有告警被频繁静默？

□ 优化行动
  - 合并/删除冗余告警
  - 调整不合理的阈值
  - 添加缺失的 SLO 告警
  - 更新 Runbook
```

## 7. 最佳实践

```
告警疲劳治理检查清单：

□ 实施 SLO-based alerting（替代技术指标告警）
□ 使用多窗口多 Burn Rate 告警
□ 配置合理的分组和抑制规则
□ 实施告警优先级分级
□ 定期审查告警健康度
□ 建立告警评审机制
□ 使用动态阈值替代静态阈值
□ 维护告警 Runbook
□ 培训团队告警响应流程
□ 持续优化告警规则
```

## Related

- [[domain-06-observability/05-alerting/01-alertmanager-deep-configuration|Alertmanager 深度配置]]
- [[domain-06-observability/05-alerting/02-pagerduty-opsgenie-integration|告警平台集成]]

## See Also

- [Google SRE Book - Alerting](https://sre.google/sre-book/practical-alerting/)
- [Prometheus Alerting Best Practices](https://prometheus.io/docs/practices/alerting/)


<!-- risk-assessed -->
