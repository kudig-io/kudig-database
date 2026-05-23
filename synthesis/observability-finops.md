---
title: 可观测性与 FinOps 的融合
description: 可观测性数据 → 资源利用率洞察 → 成本优化决策
category: synthesis
tags:
- observability
- finops
- cost-optimization
- monitoring
- sre
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可观测性与 FinOps 的融合 是什么
- 如何 可观测性与 FinOps 的融合
trigger_keywords:
- 可观测性与
- FinOps
- 的融合
prerequisites:
- kubectl-basics
- prometheus-basics
created: "2026-05-23"
relationships:
  - target: "[[best-practices/observability/monitoring]]"
    type: related_to
  - target: "[[domain-17-system-foundation/topic-dictionary/observability/observability]]"
    type: related_to
---

# 可观测性与 FinOps 的融合

## 核心思路

```
可观测性数据 → 资源利用率洞察 → 成本优化决策

示例:
  CPU 利用率 < 20% 持续 7 天 → 建议降配
  内存请求/限制比值 < 30% → 建议调整 limit
  存储增长趋势 → 预测扩容成本
```

## 标签化成本分摊

```yaml
# 统一标签策略
metadata:
  labels:
    team: platform
    project: order-service
    environment: production
    cost-center: cc-001
```

## 工具集成

```
OpenCost + Prometheus:
  → 实时计算命名空间/工作负载成本
  → 与利用率指标关联
  → 成本异常告警
```

## 相关 Domain

- [[domain-17-system-foundation/topic-dictionary/observability/observability|observability]]/02-metrics/02-[[best-practices/observability/monitoring|monitoring]]-metrics-system]]
- domain-11-production-operations/01-finops/01-cost-governance
