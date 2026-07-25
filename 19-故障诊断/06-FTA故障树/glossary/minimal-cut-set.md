---
title: 最小割集
description: 最小割集（MCS）是使顶事件发生的最小基本事件集合。移除集合中任何一个事件，顶事件就不再由该割集导致。MCS 是 FTA 定性分析的核心结果。...
summary: 最小割集（MCS）是使顶事件发生的最小基本事件集合。移除集合中任何一个事件，顶事件就不再由该割集导致。MCS 是 FTA 定性分析的核心结果。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- minimalcutset
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
- 最小割集 是什么
- Minimal Cut Set (MCS) 详解
trigger_keywords:
- 最小割集
- Minimal Cut Set (MCS)
- fta
prerequisites:
- troubleshooting-methodology
---



# 最小割集

> **英文名**: Minimal Cut Set (MCS)

## 概述

最小割集（MCS）是使顶事件发生的最小基本事件集合。移除集合中任何一个事件，顶事件就不再由该割集导致。MCS 是 FTA 定性分析的核心结果。

## 核心概念/原理

### 分析意义
- 阶数为 1 的 MCS：单点故障（最危险）。
- 阶数为 2 的 MCS：双重故障才会导致系统失效。
- MCS 阶数越低，系统风险越高。

## 关键机制或特性

MCS 分析帮助识别系统的薄弱环节和单点故障。优先处理阶数最低的 MCS 可以最有效地提升系统可靠性。

## 使用场景与最佳实践

在 K8s 中，单点故障的 MCS 示例：API Server 证书过期（1阶割集）。

## K8s 最小割集分析示例

```
顶事件: 生产服务不可用

故障树:
  T = (A AND B) OR C OR (D AND E)
  其中:
    A = apiserver 故障
    B = etcd 故障
    C = 证书过期 (单点故障!)
    D = 网络分区
    E = DNS 故障

最小割集:
  MCS-1: {C}         ← 1阶，最高优先级!
  MCS-2: {A, B}      ← 2阶
  MCS-3: {D, E}      ← 2阶

结论: 证书过期是单点故障，优先解决
  → 实施自动证书轮换 (cert-manager)
```

## 割集阶数与优先级

| 阶数 | 含义 | K8s 示例 | 优先级 |
|------|------|----------|--------|
| 1阶 | 单点故障 | 证书过期、单副本 etcd | 最高 |
| 2阶 | 双重故障 | apiserver + etcd 同时故障 | 高 |
| 3阶 | 三重故障 | 多 AZ 同时故障 | 中 |

## 消除单点故障的实践

1. **证书**: cert-manager 自动轮换 → 消除 1 阶 MCS
2. **etcd**: 3/5 节点集群 → 将 1 阶提升为 2 阶
3. **apiserver**: 多副本 + LB → 将 1 阶提升为 2 阶
4. **DNS**: 多 CoreDNS 副本 + NodeLocal DNSCache
5. **存储**: 多副本 PV + 快照备份

## 面试要点

1. **什么是最小割集？**
   - 导致顶事件发生的最小基本事件集合
   - 阶数越低，风险越高
   - 1 阶 MCS = 单点故障

2. **如何用 MCS 指导 K8s 可靠性建设？**
   - 识别所有 1 阶 MCS（单点故障）
   - 优先消除 1 阶 MCS（增加冗余）
   - 定期重新评估 MCS（架构变更后）

3. **K8s 中常见的 1 阶 MCS 有哪些？**
   - 证书过期（无自动轮换）
   - 单副本 etcd
   - 单节点控制平面
   - 无备份的有状态服务

## 参考链接

- [Minimal Cut Set (MCS)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
