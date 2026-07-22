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

## K8s 组件 FMEA 示例

| 组件 | 故障模式 | 影响 | 严重度(S) | 发生度(O) | 检测度(D) | RPN |
|------|---------|------|----------|----------|----------|-----|
| etcd | 磁盘故障 | 集群不可用 | 10 | 2 | 3 | 60 |
| apiserver | 证书过期 | API 不可用 | 9 | 4 | 2 | 72 |
| kubelet | 进程崩溃 | 节点 NotReady | 8 | 5 | 2 | 80 |
| CoreDNS | Pod 崩溃 | DNS 失败 | 7 | 3 | 4 | 84 |
| CNI | 配置错误 | 网络中断 | 9 | 3 | 5 | 135 |

## FMEA 分析流程

```
1. 列出所有组件/功能
   └─ apiserver, etcd, scheduler, kubelet, CNI, DNS...

2. 识别每个组件的故障模式
   └─ 崩溃、延迟、资源耗尽、配置错误...

3. 评估影响 (S: 1-10)
   └─ 10=集群不可用, 1=无影响

4. 评估发生概率 (O: 1-10)
   └─ 10=频繁发生, 1=极不可能

5. 评估检测难度 (D: 1-10)
   └─ 10=无法检测, 1=自动告警

6. 计算 RPN = S × O × D
   └─ RPN > 100: 优先处理
```

## FMEA 与 FTA 结合使用

```
FMEA (自底向上):
  每个组件 → 故障模式 → 影响 → RPN 排序

FTA (自顶向下):
  顶事件 → 故障树 → 最小割集 → 概率计算

结合:
  FMEA 识别高风险组件 → FTA 深入分析根因
  FTA 发现单点故障 → FMEA 评估影响范围
```

## 面试要点

1. **FMEA 的三个评估维度？**
   - 严重度(S): 故障影响程度
   - 发生度(O): 故障发生频率
   - 检测度(D): 故障发现难度
   - RPN = S × O × D

2. **FMEA 和 FTA 如何互补？**
   - FMEA: 自底向上，预防性分析
   - FTA: 自顶向下，根因分析
   - 结合使用全面覆盖风险

3. **K8s 中哪些组件 RPN 最高？**
   - CNI 配置错误（高严重度 + 难检测）
   - 证书过期（高严重度 + 高发生度）
   - 磁盘故障（高严重度 + 难检测）

## 参考链接

- [FMEA (Failure Mode and Effects Analysis)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
