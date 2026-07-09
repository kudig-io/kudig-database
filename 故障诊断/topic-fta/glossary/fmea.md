---
title: 故障模式与影响分析
description: FMEA（Failure Mode and Effects Analysis，故障模式与影响分析）是一种自底向上的归纳式分析方法。它系统地识别系统中每个组件的潜...
summary: FMEA（Failure Mode and Effects Analysis，故障模式与影响分析）是一种自底向上的归纳式分析方法。它系统地识别系统中每个组件的潜...
category: fta
tags:
- fta
- troubleshooting
- reliability
- fmea
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
- 故障模式与影响分析 是什么
- FMEA (Failure Mode and Effects Analysis) 详解
trigger_keywords:
- 故障模式与影响分析
- FMEA (Failure Mode and Effects Analysis)
- fta
prerequisites:
- troubleshooting-methodology
---



# 故障模式与影响分析

> **英文名**: FMEA (Failure Mode and Effects Analysis)

## 概述

FMEA（Failure Mode and Effects Analysis，故障模式与影响分析）是一种自底向上的归纳式分析方法。它系统地识别系统中每个组件的潜在故障模式，评估其对系统的影响，并制定预防措施。

## 核心概念/原理

### 分析步骤
1. 列出系统所有组件。
2. 识别每个组件的故障模式。
3. 评估每个故障模式的影响（严重度 S、发生频率 O、可检测性 D）。
4. 计算 RPN（风险优先级数）= S × O × D。
5. 按 RPN 排序，优先处理高风险项。

## 关键机制或特性

FMEA 与 FTA 互为补充：FTA 是自顶向下演绎，FMEA 是自底向上归纳。两者结合可以全面覆盖系统风险。

## 使用场景与最佳实践

在 K8s 中，FMEA 可用于分析每个组件（API Server、etcd、kubelet 等）的故障模式和影响。

## 参考链接

- [FMEA (Failure Mode and Effects Analysis)]()

## Related

- [[故障诊断/topic-fta/appendix-a-glossary.md|FTA 术语表]]
