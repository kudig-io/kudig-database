---
title: 外部事件
description: 外部事件（House Event）是故障树中表示正常预期会发生的事件。它不是故障，而是作为条件或触发器存在于故障树中。...
summary: 外部事件（House Event）是故障树中表示正常预期会发生的事件。它不是故障，而是作为条件或触发器存在于故障树中。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- houseevent
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
- 外部事件 是什么
- House Event 详解
trigger_keywords:
- 外部事件
- House Event
- fta
prerequisites:
- troubleshooting-methodology
---



# 外部事件

> **英文名**: House Event

## 概述

外部事件（House Event）是故障树中表示正常预期会发生的事件。它不是故障，而是作为条件或触发器存在于故障树中。

## 核心概念/原理

### 用途
- 表示系统运行模式的切换。
- 表示计划内的维护操作。
- 作为逻辑门的条件输入。

## 关键机制或特性

House Event 用于简化故障树建模，将正常操作与故障事件区分开来。

## 使用场景与最佳实践

在 K8s 中，节点维护窗口、计划性升级等可以作为 House Event 建模。

## K8s 中的房事件应用

```
房事件: 可控制的事件，用于建模安全措施

示例: 节点维护场景
  顶事件: Pod 被驱逐
      │
     [AND]
      │
      ├── 输入事件: 节点资源不足
      │
      └── 房事件: 维护模式未启用 ◇
           └─ 如果维护模式启用，则不驱逐

K8s 实践:
  kubectl cordon <node>  ← 设置房事件为维护模式
  kubectl drain <node>   ← 手动控制驱逐
  PDB 配置              ← 房事件: 最小可用副本

房事件 vs 基本事件:
  基本事件: 不可控 (硬件故障)
  房事件: 可控 (配置、策略、开关)
```

## 房事件在故障树中的表示

```
    ┌─────────┐
    │ 输出事件 │
    └────┬────┘
         │
      ┌──┴──┐
      │ AND │
      └──┬──┘
     ┌───┴───┐
     │       │
    ○       ◇  ← 房事件 (菱形/房屋形)
  基本事件  房事件
  (不可控)  (可控)
```

## 面试要点

1. **房事件和基本事件的区别？**
   - 基本事件: 不可控的根因 (硬件故障)
   - 房事件: 可控的条件 (配置、策略)
   - 房事件用于建模保护措施

2. **K8s 中哪些是房事件？**
   - 维护模式 (cordon/drain)
   - PDB 配置
   - 特性开关 (Feature Gate)
   - 网络策略开关

3. **房事件如何影响可靠性分析？**
   - 房事件可以切断故障传播路径
   - 正确配置房事件可提高可靠性
   - 需要考虑房事件失效的情况

## 参考链接

- [House Event]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
