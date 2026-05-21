---
title: Burn Rate 告警与预算消耗监控
description: '# Burn Rate 告警与预算消耗监控'
category: domain
tags:
- sre
- slo
- burn-rate
- alerting
- monitoring
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Burn Rate 告警与预算消耗监控 是什么
- 如何 Burn Rate 告警与预算消耗监控
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Burn
- Rate
- 告警与预算消耗监控
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

# Burn Rate 告警与预算消耗监控

> Burn Rate 回答"以当前速度，错误预算将在多久后耗尽？"

## Burn Rate 基础

**Burn Rate** = 当前错误率 / (1 - SLO)

| Burn Rate | 耗尽时间(30天窗口) | 告警级别 |
|-----------|------------------|---------|
| 1x | 30 天 | 正常 |
| 2x | 15 天 | 提醒 |
| 6x | 5 天 | 警告 |
| 14.4x | ~2 天 | 严重 |
| 60x | 12 小时 | 紧急 |

## 多级告警配置

```yaml
alerts:
  - name: fast_burn
    burn_rate: 14.4
    window: 1h
    severity: critical
    
  - name: medium_burn  
    burn_rate: 6
    window: 6h
    severity: warning
    
  - name: slow_burn
    burn_rate: 2
    window: 3d
    severity: info
```

## PromQL 告警规则

```promql
# Fast Burn (14.4x) - 2天内耗尽
(
  sum(rate(http_requests_total{status=~"5.."}[1h]))
  / sum(rate(http_requests_total[1h]))
) > 14.4 * (1 - 0.999)

# Slow Burn (2x) - 15天内耗尽
(
  sum(rate(http_requests_total{status=~"5.."}[3d]))
  / sum(rate(http_requests_total[3d]))
) > 2 * (1 - 0.999)
```

## 相关

- [[domain-09-reliability-engineering/04-slo-sli/03-error-budget-management]]
- [[domain-09-reliability-engineering/07-sre-practices/02-release-gate-slo-based]]
