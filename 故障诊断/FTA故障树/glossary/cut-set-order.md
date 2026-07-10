---
title: 割集阶数
description: 割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。...
summary: 割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- cutsetorder
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
- 割集阶数 是什么
- Cut Set Order 详解
trigger_keywords:
- 割集阶数
- Cut Set Order
- fta
prerequisites:
- troubleshooting-methodology
---



# 割集阶数

> **英文名**: Cut Set Order

## 概述

割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。

## 核心概念/原理

### 风险等级
- **阶数 1**：单点故障（最危险），一个事件就能导致系统故障。
- **阶数 2**：双重故障，两个事件同时发生才导致系统故障。
- **阶数 3+**：需要多个事件同时发生，概率较低。

## 关键机制或特性

消除阶数为 1 的最小割集（单点故障）是提升系统可靠性的首要目标。

## 使用场景与最佳实践

在 K8s 中，API Server 单实例部署是 1 阶割集（单点故障），应通过多副本消除。

## 参考链接

- [Cut Set Order]()

## Related

- [[故障诊断/topic-fta/appendix-a-glossary.md|FTA 术语表]]
