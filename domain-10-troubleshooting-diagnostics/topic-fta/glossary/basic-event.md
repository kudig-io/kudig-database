---
title: 基本事件
description: '基本事件（Basic Event）是故障树中不可再分解的最底层事件。它代表了导致上层事件发生的根本原因，是故障分析的终点。...'
category: fta
tags:
- fta
- troubleshooting
- reliability
- basicevent
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 基本事件 是什么
- Basic Event 详解
trigger_keywords:
- 基本事件
- Basic Event
- fta
prerequisites:
- troubleshooting-methodology
created: "2026-06-24"
---

# 基本事件

> **英文名**: Basic Event

## 概述

基本事件（Basic Event）是故障树中不可再分解的最底层事件。它代表了导致上层事件发生的根本原因，是故障分析的终点。

## 核心概念/原理

### 特征
- 不再向下分解。
- 有已知的发生概率或频率。
- 对应具体的根因（如配置错误、资源不足、网络中断）。

## 关键机制或特性

基本事件是制定修复和预防措施的依据。消除或降低基本事件的发生概率可以直接降低顶事件的发生概率。

## 使用场景与最佳实践

在 K8s FTA 中，基本事件对应具体的根因如：CPU Limit 过低、PVC 绑定失败、证书过期等。

## 参考链接

- [Basic Event]()

## Related

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary.md|FTA 术语表]]
