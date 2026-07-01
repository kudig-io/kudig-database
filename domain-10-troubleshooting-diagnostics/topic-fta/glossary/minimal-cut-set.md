---
title: 最小割集
description: '最小割集（MCS）是使顶事件发生的最小基本事件集合。移除集合中任何一个事件，顶事件就不再由该割集导致。MCS 是 FTA 定性分析的核心结果。...'
category: fta
tags:
- fta
- troubleshooting
- reliability
- minimalcutset
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 最小割集 是什么
- Minimal Cut Set (MCS) 详解
trigger_keywords:
- 最小割集
- Minimal Cut Set (MCS)
- fta
prerequisites:
- troubleshooting-methodology
created: "2026-06-24"
---

# 最小割集

> **英文名**: Minimal Cut Set (MCS)

## 概述

最小割集（MCS）是使顶事件发生的最小基本事件集合。移除集合中任何一个事件，顶事件就不再由该割集导致。MCS 是 FTA 定性分析的核心结果。

## 核心概念/原理

### 分析意义
- 阶数为 1 的 MCS：单点故障（最危险）。
- 阶数为 2 的 MCS：双重故障才会导致系统失效。
- MCS 阶数越低，系统风险越高。

## 关键机制或特性

MCS 分析帮助识别系统的薄弱环节和单点故障。优先处理阶数最低的 MCS 可以最有效地提升系统可靠性。

## 使用场景与最佳实践

在 K8s 中，单点故障的 MCS 示例：API Server 证书过期（1阶割集）。

## 参考链接

- [Minimal Cut Set (MCS)]()

## Related

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary.md|FTA 术语表]]
