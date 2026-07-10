---
title: 问题率
description: 问题率（Failure Rate，λ）是单位时间内系统或组件发生故障的概率。它是可靠性工程的基础参数。...
summary: 问题率（Failure Rate，λ）是单位时间内系统或组件发生故障的概率。它是可靠性工程的基础参数。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- failurerate
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
- 问题率 是什么
- Failure Rate (λ) 详解
trigger_keywords:
- 问题率
- Failure Rate (λ)
- fta
prerequisites:
- troubleshooting-methodology
---



# 问题率

> **英文名**: Failure Rate (λ)

## 概述

问题率（Failure Rate，λ）是单位时间内系统或组件发生故障的概率。它是可靠性工程的基础参数。

## 核心概念/原理

### 计算公式
λ = 故障次数 / 总运行时间
λ = 1 / MTBF

## 关键机制或特性

问题率通常遵循浴盆曲线（Bathtub Curve）：早期故障期（高λ）→ 稳定期（低λ）→ 耗损期（λ上升）。

## 使用场景与最佳实践

在 K8s 中，可统计各组件（API Server、etcd、kubelet）的问题率，识别不稳定组件。

## 参考链接

- [Failure Rate (λ)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
