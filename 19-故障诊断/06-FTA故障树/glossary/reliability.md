---
title: 可靠度
description: 可靠度（Reliability，R(t)）是系统在时间 t 内无故障运行的概率。它是时间相关的可靠性指标。...
summary: 可靠度（Reliability，R(t)）是系统在时间 t 内无故障运行的概率。它是时间相关的可靠性指标。...
category: fta
tags:
- fta
- troubleshooting
- reliability
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 可靠度 是什么
- Reliability R(t) 详解
trigger_keywords:
- 可靠度
- Reliability R(t)
- fta
prerequisites:
- troubleshooting-methodology
---



# 可靠度

> **英文名**: Reliability R(t)

## 概述

可靠度（Reliability，R(t)）是系统在时间 t 内无故障运行的概率。它是时间相关的可靠性指标。

## 核心概念/原理

### 计算公式
R(t) = e^(-λt)（指数分布假设）
R(t) = 1 - F(t)（F(t) 为累积故障分布函数）

## 关键机制或特性

可靠度随时间递减。系统的整体可靠度取决于各组件可靠度的组合（串联/并联）。

## 使用场景与最佳实践

在 K8s 中，评估集群在特定时间段内的可靠运行概率，指导维护计划。

## K8s 可靠性设计原则

| 原则 | K8s 实践 | 效果 |
|------|----------|------|
| 冗余 | 多副本、多 AZ | 消除单点故障 |
| 自愈 | 健康检查、自动重启 | 降低 MTTR |
| 降级 | 熔断、限流 | 防止雪崩 |
| 隔离 | Namespace、NetworkPolicy | 限制故障范围 |
| 监控 | Prometheus、告警 | 降低 MTTD |

## 可靠性计算

```
串联系统: R = R1 × R2 × ... × Rn
  例: apiserver(0.999) × etcd(0.999) × 网络(0.99)
     = 0.988 (串联降低可靠性)

并联系统: R = 1 - (1-R1)(1-R2)...(1-Rn)
  例: 3个 apiserver (各 0.999)
     = 1 - 0.001³ = 0.999999999

K8s 控制平面 HA:
  3 apiserver + 3 etcd + LB
  R ≈ 0.99999 (五个九)
```

## 面试要点

1. **可靠性和可用性的区别？**
   - 可靠性: 无故障运行概率 (R = e^(-λt))
   - 可用性: 需要时可用概率 (A = MTBF/(MTBF+MTTR))
   - 可靠性关注“不坏”，可用性关注“能用”

2. **K8s 如何提高系统可靠性？**
   - 多副本 + PDB（冗余）
   - 健康检查 + 自动重启（自愈）
   - 反亲和性（故障域分散）

3. **串联和并联对可靠性的影响？**
   - 串联: 所有组件必须工作，可靠性相乘
   - 并联: 任一工作即可，可靠性大幅提高
   - K8s HA 设计本质是并联冗余

## 参考链接

- [Reliability R(t)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
