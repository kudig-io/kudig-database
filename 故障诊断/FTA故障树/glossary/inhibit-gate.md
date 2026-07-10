---
title: 抑制门
description: 抑制门（Inhibit Gate）是带条件约束的 AND 门。输出事件仅在输入事件和条件事件同时发生时才发生。...
summary: 抑制门（Inhibit Gate）是带条件约束的 AND 门。输出事件仅在输入事件和条件事件同时发生时才发生。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- inhibitgate
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
- 抑制门 是什么
- Inhibit Gate 详解
trigger_keywords:
- 抑制门
- Inhibit Gate
- fta
prerequisites:
- troubleshooting-methodology
---



# 抑制门

> **英文名**: Inhibit Gate

## 概述

抑制门（Inhibit Gate）是带条件约束的 AND 门。输出事件仅在输入事件和条件事件同时发生时才发生。

## 核心概念/原理

### 逻辑含义
输出 = 输入事件 AND 条件事件
条件事件不是故障，而是使故障生效的外部条件。

## 关键机制或特性

抑制门用于建模条件性故障：只有在特定条件下，故障才会导致上层事件。

## 使用场景与最佳实践

在 K8s 中，Pod 调度失败 = 资源不足 AND 没有配置 PriorityClass（条件）。

## 参考链接

- [Inhibit Gate]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
