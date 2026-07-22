---
title: 投票门
description: 投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。...
summary: 投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- votinggate
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
- 投票门 是什么
- Voting Gate (k/n) 详解
trigger_keywords:
- 投票门
- Voting Gate (k/n)
- fta
prerequisites:
- troubleshooting-methodology
---



# 投票门

> **英文名**: Voting Gate (k/n)

## 概述

投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。

## 核心概念/原理

### 特殊情况
- k=1：等同于 OR 门（任一输入即触发）。
- k=n：等同于 AND 门（全部输入才触发）。
- k/n：n 中取 k 的表决逻辑。

## 关键机制或特性

投票门常用于分析冗余系统的降级模式（如 3 个节点中 2 个故障时系统不可用 = 2/3 投票门）。

## 使用场景与最佳实践

在 K8s 中，etcd 集群在 3 节点中有 2 个故障时不可用（2/3 投票门）。

## K8s 中的表决门场景

```
表决门 (k/n): n 个输入中至少 k 个发生时输出发生

K8s 示例 1: etcd Raft 共识 (2/3 表决门)
  顶事件: etcd 写入失败
      │
    [2/3 表决门]
      ├── etcd-0 故障
      ├── etcd-1 故障
      └── etcd-2 故障
  → 至少 2 个节点故障才导致写入失败

K8s 示例 2: 多副本服务 (1/3 表决门)
  顶事件: 服务容量不足
      │
    [1/3 表决门]
      ├── Pod-0 不可用
      ├── Pod-1 不可用
      └── Pod-2 不可用
  → 任一 Pod 故障即导致容量下降

概率计算:
  P(k/n) = Σ C(n,i) × P^i × (1-P)^(n-i)  (i=k to n)
  例: etcd 2/3, P(单节点故障)=0.01
  P(写入失败) = C(3,2)×0.01²×0.99 + C(3,3)×0.01³
             ≈ 0.000298
```

## 表决门与与门/或门的关系

| 门类型 | 等价表决门 | K8s 示例 |
|--------|-----------|----------|
| 或门 | 1/n 表决门 | 任一 Pod 故障 |
| 与门 | n/n 表决门 | 所有节点故障 |
| 表决门 | k/n (1<k<n) | etcd Raft 共识 |

## 面试要点

1. **表决门在 K8s 中的典型应用？**
   - etcd Raft: 2/3 或 3/5 表决门
   - 多副本服务: 1/n 表决门（容量）
   - 多 AZ: k/n 表决门（可用性）

2. **表决门如何影响可靠性计算？**
   - 比或门更可靠（不需要所有输入）
   - 比与门更敏感（不需要全部发生）
   - 用二项分布计算概率

3. **如何设计合适的表决门？**
   - 根据可用性目标选择 k/n
   - etcd 推荐 3 或 5 节点（奇数）
   - 服务副本数根据 SLO 确定

## 参考链接

- [Voting Gate (k/n)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
