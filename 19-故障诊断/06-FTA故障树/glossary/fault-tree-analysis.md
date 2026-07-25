---
title: 故障树分析
description: 故障树分析（FTA）是一种自顶向下的演绎式系统安全分析方法。它通过逻辑门将系统级故障（顶事件）分解为底层基本事件的组合，用于识别导致系统故障的根本原因和传播路径...
summary: 故障树分析（FTA）是一种自顶向下的演绎式系统安全分析方法。它通过逻辑门将系统级故障（顶事件）分解为底层基本事件的组合，用于识别导致系统故障的根本原因和传播路径...
category: fta
tags:
- fta
- troubleshooting
- reliability
- faulttreeanalysis
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
- 故障树分析 是什么
- Fault Tree Analysis (FTA) 详解
trigger_keywords:
- 故障树分析
- Fault Tree Analysis (FTA)
- fta
prerequisites:
- troubleshooting-methodology
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

## FTA 在 K8s 运维中的应用流程

```
1. 定义顶事件
   └─ 例: "生产 API 服务 P99 延迟 > 2s 持续 5min"

2. 构建故障树
   └─ 自顶向下分解: 顶事件 → 中间事件 → 基本事件
   └─ 使用 OR/AND 门连接

3. 定性分析
   └─ 识别最小割集 (MCS)
   └─ 找出单点故障 (1阶 MCS)

4. 定量分析
   └─ 为基本事件赋予概率
   └─ 计算顶事件概率

5. 制定改进措施
   └─ 优先消除低阶 MCS
   └─ 降低高概率基本事件
```

## K8s FTA 实战示例

```
顶事件: Pod 无法访问外部服务
    │
   [OR]
    ├── DNS 解析失败
    │    [OR]
    │    ├── CoreDNS Pod 崩溃
    │    ├── NetworkPolicy 阻止 DNS
    │    └── 上游 DNS 不可达
    │
    ├── 网络不可达
    │    [OR]
    │    ├── CNI 故障
    │    ├── 防火墙规则
    │    └── 路由表错误
    │
    └── 目标服务不可用
         [OR]
         ├── 目标 Pod 崩溃
         ├── Service selector 错误
         └── 目标端口未监听
```

## FTA 与其他方法对比

| 方法 | 方向 | 适用场景 |
|------|------|----------|
| FTA | 自顶向下 | 已知故障，分析根因 |
| FMEA | 自底向上 | 未知故障，预防分析 |
| 5 Whys | 线性追问 | 简单故障快速定位 |
| 事件树 | 正向推演 | 分析故障后果 |

## 面试要点

1. **FTA 的核心步骤？**
   - 定义顶事件 → 构建故障树 → 定性分析(MCS) → 定量分析(概率) → 改进

2. **FTA 和 FMEA 如何互补？**
   - FTA: 自顶向下，已知故障找根因
   - FMEA: 自底向上，每个组件的故障影响
   - 结合使用可全面覆盖风险

3. **K8s 中何时使用 FTA？**
   - 生产事故复盘（根因分析）
   - 新架构风险评估（预防分析）
   - SLO 不达标时的系统性排查

## 参考链接

- [Fault Tree Analysis (FTA)](https://en.wikipedia.org/wiki/Fault_tree_analysis)

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
