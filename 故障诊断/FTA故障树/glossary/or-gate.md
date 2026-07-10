---
title: 或门
description: 或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。...
summary: 或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- orgate
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
- 或门 是什么
- OR Gate 详解
trigger_keywords:
- 或门
- OR Gate
- fta
prerequisites:
- troubleshooting-methodology
---



# 或门

> **英文名**: OR Gate

## 概述

或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。

## 核心概念/原理

### 逻辑含义
输出 = 输入1 OR 输入2 OR ... OR 输入N
任一输入发生，输出就发生。所有输入都不发生，输出才不发生。

## 关键机制或特性

或门使故障概率增大（P = 1 - ∏(1-Pi)），是系统脆弱性的标志。多个独立故障路径通过或门汇聚意味着系统缺乏冗余。

## 使用场景与最佳实践

在 K8s 中，Service 不可用可能是因为所有后端 Pod 不可用 OR 网络不通 OR DNS 解析失败。

## 参考链接

- [OR Gate]()

## Related

- [[故障诊断/topic-fta/appendix-a-glossary.md|FTA 术语表]]
