---
title: 重要度
description: 重要度（Importance Measure）衡量基本事件对顶事件的影响程度。它是 FTA 定量分析的关键指标，用于确定哪些基本事件最值得改进。...
summary: 重要度（Importance Measure）衡量基本事件对顶事件的影响程度。它是 FTA 定量分析的关键指标，用于确定哪些基本事件最值得改进。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- importancemeasure
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
- 重要度 是什么
- Importance Measure 详解
trigger_keywords:
- 重要度
- Importance Measure
- fta
prerequisites:
- troubleshooting-methodology
---



# 重要度

> **英文名**: Importance Measure

## 概述

重要度（Importance Measure）衡量基本事件对顶事件的影响程度。它是 FTA 定量分析的关键指标，用于确定哪些基本事件最值得改进。

## 核心概念/原理

### 常见重要度指标
- **Birnbaum 重要度**：基本事件状态改变对顶事件概率的影响。
- **Fussell-Vesely 重要度**：基本事件参与的割集对顶事件的贡献比例。
- **关键度（Criticality）**：Birnbaum 重要度 × 基本事件概率 / 顶事件概率。

## 关键机制或特性

重要度分析帮助确定资源投入的优先级。重要度最高的基本事件应优先改进，可以最有效地降低顶事件发生概率。

## 使用场景与最佳实践

在 K8s FTA 中，重要度分析可以识别最影响系统可用性的根因事件。

## 参考链接

- [Importance Measure]()

## Related

- [[故障诊断/topic-fta/appendix-a-glossary.md|FTA 术语表]]
