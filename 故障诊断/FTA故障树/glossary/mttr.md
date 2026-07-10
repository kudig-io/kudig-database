---
title: 平均修复时间
description: MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR
  越短，系统恢复能...
summary: MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR 越短，系统恢复能...
category: fta
tags:
- fta
- troubleshooting
- reliability
- mttr
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
- 平均修复时间 是什么
- MTTR (Mean Time To Repair) 详解
trigger_keywords:
- 平均修复时间
- MTTR (Mean Time To Repair)
- fta
prerequisites:
- troubleshooting-methodology
---



# 平均修复时间

> **英文名**: MTTR (Mean Time To Repair)

## 概述

MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR 越短，系统恢复能力越强。

## 核心概念/原理

### 计算公式
MTTR = 总修复时间 / 故障次数
MTTR = MTTD + 诊断时间 + 修复时间 + 验证时间

## 关键机制或特性

降低 MTTR 是运维工程的核心目标。通过自动化诊断（AI Agent）、预定义修复方案（Runbook）和自动修复可以显著降低 MTTR。

## 使用场景与最佳实践

在 K8s 中，MTTR 包括：发现告警→定位根因→执行修复→验证恢复的总时间。

## 参考链接

- [MTTR (Mean Time To Repair)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
