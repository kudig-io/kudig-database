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

## K8s 组件故障率参考

| 组件 | 典型故障率 | 单位 | 监控指标 |
|------|----------|------|----------|
| apiserver | 0.1-0.5 | 次/月 | request_duration_seconds |
| etcd | 0.05-0.2 | 次/月 | wal_fsync_duration |
| kubelet | 0.5-2 | 次/月/节点 | node_ready_status |
| Pod (无状态) | 1-5 | 次/月/服务 | restart_count |
| CNI 插件 | 0.1-0.5 | 次/月 | pod_network_errors |

## 故障率与可靠性计算

```
故障率 λ = 故障次数 / 总运行时间
MTBF = 1 / λ

示例: kubelet 故障率
  100 个节点，运行 30 天
  发生 10 次 kubelet 崩溃
  λ = 10 / (100 × 30 × 24) = 0.000139/h
  MTBF = 1/0.000139 = 7200h ≈ 300天

浴盆曲线:
  早期故障期 → 偶然故障期(恒定λ) → 耗损故障期
  K8s 组件通常在偶然故障期运行
```

## 面试要点

1. **故障率和 MTBF 的关系？**
   - λ = 1/MTBF，互为倒数
   - 故障率越低，MTBF 越高，系统越可靠

2. **如何降低 K8s 组件故障率？**
   - 定期更新和补丁
   - 资源监控和预警
   - 冗余设计降低影响

3. **浴盆曲线在 K8s 中的体现？**
   - 早期：新部署的配置错误（早期故障期）
   - 中期：稳定运行（偶然故障期）
   - 后期：证书过期、磁盘老化（耗损故障期）

## 参考链接

- [Failure Rate (λ)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
