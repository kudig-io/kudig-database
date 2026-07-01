---
title: SLO 与监控系统的深度集成
description: '# SLO 与监控系统的深度集成'
category: synthesis
tags:
- slo
- monitoring
- observability
- reliability
- alerting
- prometheus
- grafana
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SLO 与监控系统的深度集成 是什么
- 如何 SLO 与监控系统的深度集成
trigger_keywords:
- SLO
- 与监控系统的深度集成
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
relationships:
  - target: "[[skills/best-practices/best-practices/observability/monitoring.md]]"
    type: related_to
  - target: "[[domain-17-system-foundation/topic-dictionary/observability/observability.md]]"
    type: related_to
  - target: "[[domain-17-system-foundation/topic-cheat-sheet/gitops.md]]"
    type: related_to
---

# SLO 与监控系统的深度集成

## 架构概览

```
用户请求 → Ingress → Service → Pod
                ↓
         Prometheus (SLI 指标)
                ↓
    ┌─────────────────────────┐
    │   SLO 计算引擎           │
    │  (错误预算、Burn Rate)   │
    └─────────────────────────┘
                ↓
    ┌─────────────────────────┐
    │   Grafana 看板           │
    │   Alertmanager 告警      │
    │   CI/CD 发布门控         │
    └─────────────────────────┘
```

## 集成要点

```
1. SLI 指标标准化
   → 统一命名: slo_service_latency_p99
   → 统一标签: service, slo_name, window

2. 错误预算自动计算
   → Recording Rules 实时计算
   → 多窗口聚合

3. 告警分级
   → Fast Burn (14.4x) → Page
   → Slow Burn (2x) → Ticket

4. 发布门控
   → CI/CD 调用 SLO API
   → 预算不足时阻止发布
```

## 相关 Domain

- [[domain-09-reliability-engineering/04-slo-sli/01-sli-definition-selection.md|01 sli definition selection]]
- [[domain-17-system-foundation/topic-dictionary/observability/observability.md|observability]]/02-metrics/02-[[skills/best-practices/best-practices/observability/monitoring.md|monitoring]]-metrics-system]]
- domain-08-release-change-management/01-[[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]/01-gitops-principles
## Related

- [[domain-17-system-foundation/topic-cheat-sheet/git.md|Git 速查卡]]
