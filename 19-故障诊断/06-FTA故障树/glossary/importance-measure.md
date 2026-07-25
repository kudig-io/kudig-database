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

## K8s 中的重要性度量应用

```
重要性度量: 识别对顶事件影响最大的基本事件

示例: Service 不可用故障树
  基本事件重要性排序:
  1. CNI 故障 (I=0.35) ← 最优先修复
  2. DNS 失败 (I=0.25)
  3. Pod 崩溃 (I=0.20)
  4. 证书过期 (I=0.12)
  5. 配置错误 (I=0.08)

决策:
  → 优先投资 CNI 可靠性和监控
  → 其次优化 DNS 架构
```

## 重要性度量类型

| 类型 | 含义 | 用途 |
|------|------|------|
| Birnbaum | 基本事件概率变化对顶事件的影响 | 识别关键组件 |
| Fussell-Vesely | 基本事件在割集中的贡献 | 识别单点故障 |
| 风险降低 | 消除基本事件后顶事件概率降低 | 指导改进优先级 |

## 面试要点

1. **重要性度量的作用？**
   - 识别对系统可靠性影响最大的组件
   - 指导资源投入优先级
   - 量化改进效果

2. **K8s 中如何应用重要性度量？**
   - 分析历史故障数据，识别高频根因
   - 优先投资高重要性组件的可靠性
   - 验证改进措施的效果

3. **如何计算重要性度量？**
   - Birnbaum: I = ∂P(顶)/∂P(基本事件)
   - 实践中用故障树分析软件计算
   - 定期更新（基于实际故障统计）

## 参考链接

- [Importance Measure]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
