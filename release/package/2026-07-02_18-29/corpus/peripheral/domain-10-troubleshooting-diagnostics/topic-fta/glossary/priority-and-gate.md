---
title: 优先与门
description: 优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。...
summary: 优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- priorityandgate
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
- 优先与门 是什么
- Priority AND Gate (PAND) 详解
trigger_keywords:
- 优先与门
- Priority AND Gate (PAND)
- fta
prerequisites:
- troubleshooting-methodology
---



# 优先与门

> **英文名**: Priority AND Gate (PAND)

## 概述

优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。

## 核心概念/原理

### 逻辑含义
输出 = 输入1 先发生 THEN 输入2 发生
事件发生的顺序很重要。

## 关键机制或特性

PAND 用于分析时序敏感的故障场景：某些故障只在特定操作顺序下才会导致问题。

## 使用场景与最佳实践

在 K8s 中，数据丢失 = 先删除 PVC 再执行备份（顺序敏感）。

## 参考链接

- [Priority AND Gate (PAND)]()

## Related

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary.md|FTA 术语表]]
