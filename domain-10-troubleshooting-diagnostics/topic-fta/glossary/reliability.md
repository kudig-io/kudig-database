---
title: 可靠度
description: '可靠度（Reliability，R(t)）是系统在时间 t 内无故障运行的概率。它是时间相关的可靠性指标。...'
category: fta
tags:
- fta
- troubleshooting
- reliability
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
created: "2026-06-24"
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

## 参考链接

- [Reliability R(t)]()

## Related

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary.md|FTA 术语表]]
