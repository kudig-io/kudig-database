---
title: 可用性计算模型
description: 可用性计算模型 — Kubernetes 生产运维知识库
category: domain
tags:
- sre
- availability
- reliability
- sla
- calculation
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可用性计算模型 是什么
- 如何 可用性计算模型
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 可用性计算模型
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

# 可用性计算模型

## 基本公式

### 基于时间的可用性

```
可用性 = 正常运行时间 / (正常运行时间 + 停机时间)

或:
可用性 = MTBF / (MTBF + MTTR)

其中:
  MTBF = Mean Time Between Failures (平均故障间隔)
  MTTR = Mean Time To Recovery (平均恢复时间)
```

### 基于请求的可用性

```
可用性 = 成功请求数 / 总请求数

或:
可用性 = 1 - 错误率
```

## 多服务综合可用性

### 串联系统

```
服务 A (99.9%) → 服务 B (99.95%) → 服务 C (99.9%)

综合可用性 = 99.9% × 99.95% × 99.9%
           = 0.999 × 0.9995 × 0.999
           ≈ 99.75%
```

### 并联系统（冗余）

```
服务 A1 (99.9%) 
           ╲
            → 负载均衡 → 综合可用性
           ╱
服务 A2 (99.9%)

综合可用性 = 1 - (1 - 99.9%)²
           = 1 - 0.001²
           = 99.9999%
```

## 可用性目标对照

| 等级 | 可用性 | 年停机 | 月停机 | 周停机 |
|------|--------|--------|--------|--------|
| 2 个 9 | 99% | 3.65 天 | 7.3 小时 | 1.68 小时 |
| 3 个 9 | 99.9% | 8.76 小时 | 43.8 分钟 | 10.1 分钟 |
| 4 个 9 | 99.99% | 52.6 分钟 | 4.38 分钟 | 1.01 分钟 |
| 5 个 9 | 99.999% | 5.26 分钟 | 26.3 秒 | 6.05 秒 |

## 相关

- [[domain-09-reliability-engineering/04-slo-sli/02-slo-implementation-guide]]
- [[domain-09-reliability-engineering/07-sre-practices/02-release-gate-slo-based]]
