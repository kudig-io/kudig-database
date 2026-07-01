---
title: 故障树分析
description: '故障树分析（FTA）是一种自顶向下的演绎式系统安全分析方法。它通过逻辑门将系统级故障（顶事件）分解为底层基本事件的组合，用于识别导致系统故障的根本原因和传播路径...'
category: fta
tags:
- fta
- troubleshooting
- reliability
- faulttreeanalysis
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 故障树分析 是什么
- Fault Tree Analysis (FTA) 详解
trigger_keywords:
- 故障树分析
- Fault Tree Analysis (FTA)
- fta
prerequisites:
- troubleshooting-methodology
created: "2026-06-24"
---

# 故障树分析

> **英文名**: Fault Tree Analysis (FTA)

## 概述

故障树分析（FTA）是一种自顶向下的演绎式系统安全分析方法。它通过逻辑门将系统级故障（顶事件）分解为底层基本事件的组合，用于识别导致系统故障的根本原因和传播路径。

## 核心概念/原理

### 核心思想
从一个已知的系统故障（顶事件）出发，逐层向下分析导致该故障的所有可能原因和路径，直到找到不可再分解的基本事件（根因）。
### 分析流程
1. 定义顶事件 → 2. 构建故障树 → 3. 定性分析（最小割集）→ 4. 定量分析（概率计算）→ 5. 制定改进措施

## 关键机制或特性

FTA 由贝尔实验室的 H.A. Watson 于 1962 年发明，最初用于民兵导弹系统的安全分析。现广泛应用于航空航天、核工业、化工和 IT 运维领域。

## 使用场景与最佳实践

用于分析 Kubernetes 集群故障、服务不可用根因、系统性风险识别。

## 参考链接

- [Fault Tree Analysis (FTA)](https://en.wikipedia.org/wiki/Fault_tree_analysis)

## Related

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary.md|FTA 术语表]]
