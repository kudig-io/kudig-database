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

## K8s 中的禁止门场景

```
禁止门示例: 节点维护期间的 Pod 驱逐

输出事件: Pod 被驱逐
    │
  [禁止门]
    │
    ├── 输入事件: 节点资源不足
    │
    └── 禁止条件: 节点处于维护模式 (cordon)
         └─ 维护模式下不触发驱逐

K8s 实践:
  kubectl cordon <node>  ← 设置禁止条件
  kubectl drain <node>   ← 手动触发驱逐
  kubectl uncordon <node> ← 解除禁止条件
```

## 禁止门与条件门对比

| 门类型 | 触发条件 | K8s 示例 |
|--------|---------|----------|
| 禁止门 | 输入 + 禁止条件 | 维护模式不驱逐 |
| 条件门 | 输入 + 条件满足 | PDB 阻止驱逐 |
| 与门 | 所有输入 | 多条件同时满足 |

## 面试要点

1. **禁止门在故障树中的作用？**
   - 表示特定条件下故障不会传播
   - 用于建模保护机制和安全措施

2. **K8s 中哪些机制类似禁止门？**
   - PDB: 禁止同时驱逐过多 Pod
   - cordon: 禁止新 Pod 调度到节点
   - 维护窗口: 禁止特定时间执行变更

3. **禁止门如何影响可靠性分析？**
   - 降低顶事件概率（保护机制生效时）
   - 需要考虑保护机制失效的情况

## 参考链接

- [Inhibit Gate]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
