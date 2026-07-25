---
title: 基本事件
description: 基本事件（Basic Event）是故障树中不可再分解的最底层事件。它代表了导致上层事件发生的根本原因，是故障分析的终点。...
summary: 基本事件（Basic Event）是故障树中不可再分解的最底层事件。它代表了导致上层事件发生的根本原因，是故障分析的终点。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- basicevent
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
- 基本事件 是什么
- Basic Event 详解
trigger_keywords:
- 基本事件
- Basic Event
- fta
prerequisites:
- troubleshooting-methodology
---



# 基本事件

> **英文名**: Basic Event

## 概述

基本事件（Basic Event）是故障树中不可再分解的最底层事件。它代表了导致上层事件发生的根本原因，是故障分析的终点。

## 核心概念/原理

### 特征
- 不再向下分解。
- 有已知的发生概率或频率。
- 对应具体的根因（如配置错误、资源不足、网络中断）。

## 关键机制或特性

基本事件是制定修复和预防措施的依据。消除或降低基本事件的发生概率可以直接降低顶事件的发生概率。

## 使用场景与最佳实践

在 K8s FTA 中，基本事件对应具体的根因如：CPU Limit 过低、PVC 绑定失败、证书过期等。

## K8s 常见基本事件分类

| 类别 | 基本事件示例 | 典型概率 |
|------|------------|----------|
| 资源 | CPU/Memory limit 过低 | 0.1-0.3 |
| 配置 | YAML 语法错误、selector 不匹配 | 0.2-0.4 |
| 网络 | DNS 解析失败、CNI 故障 | 0.05-0.15 |
| 存储 | PVC Pending、CSI 驱动故障 | 0.05-0.1 |
| 证书 | 证书过期、CA 不信任 | 0.02-0.05 |
| 镜像 | 拉取失败、tag 不存在 | 0.1-0.2 |
| 节点 | 磁盘压力、kubelet 崩溃 | 0.05-0.1 |

## 基本事件在故障树中的表示

```
    ┌─────────┐
    │ 中间事件 │
    └────┬────┘
         │
      ┌──┴──┐
      │ OR  │
      └──┬──┘
     ┌───┼───┐
     │   │   │
    ○   ○   ○  ← 基本事件（圆形）
   E1  E2  E3

基本事件 = 不再向下分解的根因
概率来源: 历史数据 / 行业基准 / 专家估算
```

## 基本事件概率估算方法

1. **历史数据法**: 从监控系统统计过去 N 个月的故障频率
2. **行业基准**: 参考云厂商 SLA、硬件故障率数据
3. **专家估算**: 基于经验的 Delphi 法估算
4. **FMEA 转换**: 从 FMEA 的 RPN 值推导概率

## 面试要点

1. **基本事件和中间事件的区别？**
   - 基本事件：故障树的叶子节点，不再向下分解
   - 中间事件：可以进一步分解为子事件
   - 基本事件是根因分析的最终目标

2. **如何确定基本事件的概率？**
   - 优先使用历史监控数据
   - 无数据时用行业基准或专家估算
   - 定期更新概率值（基于实际故障统计）

3. **K8s 中最常见的基本事件有哪些？**
   - 配置错误（最高频）
   - 资源不足（CPU/Memory/Disk）
   - 网络问题（DNS/CNI/防火墙）
   - 证书过期（周期性故障）

## 参考链接

- [Basic Event]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
