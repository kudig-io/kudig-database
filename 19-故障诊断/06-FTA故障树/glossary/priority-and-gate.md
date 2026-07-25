---
title: 优先与门
description: 优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。...
summary: 优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- priorityandgate
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
- 优先与门 是什么
- Priority AND Gate (PAND) 详解
trigger_keywords:
- 优先与门
- Priority AND Gate (PAND)
- fta
prerequisites:
- troubleshooting-methodology
---



# 优先与门

> **英文名**: Priority AND Gate (PAND)

## 概述

优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。

## 核心概念/原理

### 逻辑含义
输出 = 输入1 先发生 THEN 输入2 发生
事件发生的顺序很重要。

## 关键机制或特性

PAND 用于分析时序敏感的故障场景：某些故障只在特定操作顺序下才会导致问题。

## 使用场景与最佳实践

在 K8s 中，数据丢失 = 先删除 PVC 再执行备份（顺序敏感）。

## K8s 中的优先与门场景

```
优先与门: 输入事件必须按特定顺序发生

K8s 示例: 数据丢失场景
  顶事件: 数据不可恢复
      │
    [优先与门]
      │
      ├── 1. 主节点磁盘故障 (先发生)
      │
      └── 2. 备份恢复失败 (后发生)
           └─ 如果备份先失败，主节点故障不会导致数据丢失

顺序重要性:
  正确顺序: 主故障 → 备份失败 = 数据丢失
  错误顺序: 备份失败 → 主故障 = 可修复(主节点还在)
```

## 优先与门与普通与门的区别

| 门类型 | 条件 | 概率影响 |
|--------|------|----------|
| 与门 | 所有输入发生 | P = P1 × P2 |
| 优先与门 | 按顺序发生 | P < P1 × P2 |

优先与门概率更低，因为顺序约束减少了故障组合。

## 面试要点

1. **优先与门的作用？**
   - 建模顺序相关的故障场景
   - 某些故障只有在特定顺序下才导致严重后果

2. **K8s 中哪些场景需要优先与门？**
   - 数据丢失: 主故障 → 备份失败
   - 服务中断: 主节点故障 → 故障转移失败
   - 安全事件: 漏洞利用 → 检测失败

3. **优先与门如何影响可靠性分析？**
   - 降低顶事件概率（顺序约束）
   - 强调时序监控的重要性
   - 指导故障恢复流程设计

## 参考链接

- [Priority AND Gate (PAND)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
