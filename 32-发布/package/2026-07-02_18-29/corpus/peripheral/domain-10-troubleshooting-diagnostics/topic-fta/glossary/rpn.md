---
title: 风险优先级数
description: RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。...
summary: RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- rpn
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
- 风险优先级数 是什么
- RPN (Risk Priority Number) 详解
trigger_keywords:
- 风险优先级数
- RPN (Risk Priority Number)
- fta
prerequisites:
- troubleshooting-methodology
---



# 风险优先级数

> **英文名**: RPN (Risk Priority Number)

## 概述

RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。

## 核心概念/原理

### 计算公式
RPN = S × O × D
- S (Severity)：严重度（1-10，10 最严重）
- O (Occurrence)：发生频率（1-10，10 最频繁）
- D (Detection)：可检测性（1-10，10 最难检测）
RPN 范围：1-1000

## 关键机制或特性

RPN 用于对故障模式进行优先级排序。高 RPN 值的故障模式应优先处理。但需注意：即使 RPN 中等，严重度极高的故障也应优先处理。

## 使用场景与最佳实践

在 K8s 运维中，用 RPN 评估不同故障场景的风险等级，指导应急预案和资源投入。

## 参考链接

- [RPN (Risk Priority Number)]()

## Related

- [[domain-10-troubleshooting-diagnostics/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
