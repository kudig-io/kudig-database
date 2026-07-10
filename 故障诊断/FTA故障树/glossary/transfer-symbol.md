---
title: 转移符号
description: 转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。...
summary: 转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- transfersymbol
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
- 转移符号 是什么
- Transfer Symbol 详解
trigger_keywords:
- 转移符号
- Transfer Symbol
- fta
prerequisites:
- troubleshooting-methodology
---



# 转移符号

> **英文名**: Transfer Symbol

## 概述

转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。

## 核心概念/原理

### 类型
- **转入（Transfer In）**：三角形 + 标签，引用其他位置的子树。
- **转出（Transfer Out）**：三角形 + 标签，定义可被引用的子树。

## 关键机制或特性

转移符号使大型故障树可以模块化，便于团队协作和分阶段构建。

## 使用场景与最佳实践

在 K8s FTA 中，各领域的故障树（网络、存储、调度）可以通过转移符号互联。

## 参考链接

- [Transfer Symbol]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
