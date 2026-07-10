---
title: 投票门
description: 投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。...
summary: 投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- votinggate
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
- 投票门 是什么
- Voting Gate (k/n) 详解
trigger_keywords:
- 投票门
- Voting Gate (k/n)
- fta
prerequisites:
- troubleshooting-methodology
---



# 投票门

> **英文名**: Voting Gate (k/n)

## 概述

投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。

## 核心概念/原理

### 特殊情况
- k=1：等同于 OR 门（任一输入即触发）。
- k=n：等同于 AND 门（全部输入才触发）。
- k/n：n 中取 k 的表决逻辑。

## 关键机制或特性

投票门常用于分析冗余系统的降级模式（如 3 个节点中 2 个故障时系统不可用 = 2/3 投票门）。

## 使用场景与最佳实践

在 K8s 中，etcd 集群在 3 节点中有 2 个故障时不可用（2/3 投票门）。

## 参考链接

- [Voting Gate (k/n)]()

## Related

- [[domain-10-troubleshooting-diagnostics/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
