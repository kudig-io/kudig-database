---
title: 转移符号
description: 转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。...
summary: 转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- transfersymbol
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
- 转移符号 是什么
- Transfer Symbol 详解
trigger_keywords:
- 转移符号
- Transfer Symbol
- fta
prerequisites:
- troubleshooting-methodology
---



# 转移符号

> **英文名**: Transfer Symbol

## 概述

转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。

## 核心概念/原理

### 类型
- **转入（Transfer In）**：三角形 + 标签，引用其他位置的子树。
- **转出（Transfer Out）**：三角形 + 标签，定义可被引用的子树。

## 关键机制或特性

转移符号使大型故障树可以模块化，便于团队协作和分阶段构建。

## 使用场景与最佳实践

在 K8s FTA 中，各领域的故障树（网络、存储、调度）可以通过转移符号互联。

## K8s 中的转移符号应用

```
转移符号: 连接到另一棵故障树或子树

示例: 大型 K8s 故障树分解

主故障树: 集群不可用
    │
   [OR]
    ├── 控制平面故障 ──▶ [转入: 控制平面故障树]
    │
    ├── 网络故障 ──▶ [转入: 网络故障树]
    │
    └── 存储故障 ──▶ [转入: 存储故障树]

控制平面故障树 (子树):
    │
   [OR]
    ├── apiserver 崩溃
    ├── etcd 故障
    └── 证书过期

优势:
  - 模块化: 每个子系统独立分析
  - 可复用: 子树可在多处引用
  - 可维护: 更新子树不影响主树
```

## 转移符号类型

| 符号 | 含义 | 用途 |
|------|------|------|
| 三角形 (向下) | 转入子树 | 分解复杂故障 |
| 三角形 (向上) | 从子树转出 | 返回主树 |
| 菱形 | 未展开事件 | 简化分析 |

## 面试要点

1. **转移符号的作用？**
   - 模块化故障树，便于管理和复用
   - 分解复杂系统为可管理的子树
   - 支持团队协作分析

2. **K8s 中如何分解故障树？**
   - 按组件: 控制平面/网络/存储/工作负载
   - 按层次: 基础设施/平台/应用
   - 每个子树独立分析和维护

3. **何时使用转移符号？**
   - 子树复杂度高（>10 个基本事件）
   - 子树可复用（多个父树引用）
   - 不同团队负责不同子系统

## 参考链接

- [Transfer Symbol]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
