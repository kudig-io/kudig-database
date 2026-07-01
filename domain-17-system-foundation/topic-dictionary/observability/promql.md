---
title: PromQL
description: 'PromQL（Prometheus Query Language）是 Prometheus 监控系统内置的查询语言，用于实时查询和分析时间序列数据。它是云原生可...'
category: dictionary
tags:
- k8s
- glossary
- promql
- prometheus
- observability
- query-language
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- PromQL 是什么
- PromQL 详解
trigger_keywords:
- PromQL
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# PromQL

> **英文名**: PromQL

## 概述

PromQL（Prometheus Query Language）是 Prometheus 监控系统内置的查询语言，用于实时查询和分析时间序列数据。它是云原生可观测性领域最重要的技能之一。

## 核心概念/原理

### 核心语法

```
# 瞬时向量（当前值）
http_requests_total{method="GET"}

# 范围向量（过去5分钟）
http_requests_total{method="GET"}[5m]

# 函数
rate(http_requests_total[5m])     # 每秒速率
sum(rate(http_requests_total[5m])) by (service)  # 按服务聚合
histogram_quantile(0.99, rate(duration_bucket[5m]))  # P99 延迟
```

### 常用函数

| 函数 | 用途 |
|------|------|
| `rate()` | 计数器每秒增长率 |
| `increase()` | 时间段内的增量 |
| `histogram_quantile()` | 分位数计算 |
| `label_replace()` | 标签改写 |

## 关键机制或特性

- **瞬时向量 vs 范围向量**：`metric` 返回最新值，`metric[5m]` 返回时间范围。
- **聚合运算符**：`sum`、`avg`、`max`、`min`、`count`、`topk` 等。
- **二元运算符**：支持向量之间的加减乘除和匹配。
- **子查询**：`rate(metric[5m])[30m:1m]` 嵌套查询。
- Recording Rules 预计算复杂查询减少查询延迟。

## 使用场景与最佳实践

- 掌握 PromQL 是 SRE/运维工程师的必备技能。
- 使用 `rate()` 而非 `irate()` 用于告警（更平滑）。
- 配置 Recording Rules 预计算常用的高开销查询。
- 使用 Grafana 变量实现动态 PromQL 查询。
- 了解 PromQL 的 Staleness 机制（5 分钟过期标记）。

## 参考链接

- [PromQL Reference](https://prometheus.io/docs/prometheus/latest/querying/basics/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]
- [[domain-17-system-foundation/topic-dictionary/observability/thanos.md|Thanos]]
- [[domain-17-system-foundation/topic-dictionary/observability/metrics-server.md|Metrics Server]]
