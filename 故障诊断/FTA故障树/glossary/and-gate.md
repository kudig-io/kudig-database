---
title: 与门
description: 与门（AND Gate）是故障树中的逻辑门，表示所有输入事件同时发生时输出事件才会发生。它代表了冗余系统中的保护机制。...
summary: 与门（AND Gate）是故障树中的逻辑门，表示所有输入事件同时发生时输出事件才会发生。它代表了冗余系统中的保护机制。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- andgate
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
- 与门 是什么
- AND Gate 详解
trigger_keywords:
- 与门
- AND Gate
- fta
prerequisites:
- troubleshooting-methodology
---



# 与门

> **英文名**: AND Gate

## 概述

与门（AND Gate）是故障树中的逻辑门，表示所有输入事件同时发生时输出事件才会发生。它代表了冗余系统中的保护机制。

## 核心概念/原理

### 逻辑含义
输出 = 输入1 AND 输入2 AND ... AND 输入N
所有输入都发生，输出才发生。任一输入不发生，输出就不发生。

## 关键机制或特性

与门使故障概率减小（P = ∏Pi），是冗余设计的体现。通过引入与门可以增加系统可靠性。

## 使用场景与最佳实践

在 K8s 中，etcd 数据丢失 = 主节点磁盘故障 AND 所有备份不可用。

## 参考链接

- [AND Gate]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
