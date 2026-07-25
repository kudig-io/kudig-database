---
title: 平均问题间隔
description: MTBF（Mean Time Between Failures，平均故障间隔时间）是衡量系统可靠性的核心指标，表示系统两次故障之间的平均运行时间。MTBF
  越长...
summary: MTBF（Mean Time Between Failures，平均故障间隔时间）是衡量系统可靠性的核心指标，表示系统两次故障之间的平均运行时间。MTBF
  越长...
category: fta
tags:
- fta
- troubleshooting
- reliability
- mtbf
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
- 平均问题间隔 是什么
- MTBF (Mean Time Between Failures) 详解
trigger_keywords:
- 平均问题间隔
- MTBF (Mean Time Between Failures)
- fta
prerequisites:
- troubleshooting-methodology
---



# 平均问题间隔

> **英文名**: MTBF (Mean Time Between Failures)

## 概述

MTBF（Mean Time Between Failures，平均故障间隔时间）是衡量系统可靠性的核心指标，表示系统两次故障之间的平均运行时间。MTBF 越长，系统越可靠。

## 核心概念/原理

### 计算公式
MTBF = 总运行时间 / 故障次数
MTBF = 1 / λ （λ 为故障率）

## 关键机制或特性

MTBF 用于评估系统组件的可靠性，指导维护计划和备件策略。

## 使用场景与最佳实践

在 K8s 中，可统计集群平均无故障运行天数、Pod 平均重启间隔等。

## 参考链接

- [MTBF (Mean Time Between Failures)]()

## Related

- [[domain-10-troubleshooting-diagnostics/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
