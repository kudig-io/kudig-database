---
title: 可用性
description: 可用性（Availability）是系统正常运行时间占总时间的比例，通常以百分比表示。它是系统可靠性和恢复能力的综合指标。...
summary: 可用性（Availability）是系统正常运行时间占总时间的比例，通常以百分比表示。它是系统可靠性和恢复能力的综合指标。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- availability
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
- 可用性 是什么
- Availability 详解
trigger_keywords:
- 可用性
- Availability
- fta
prerequisites:
- troubleshooting-methodology
---



# 可用性

> **英文名**: Availability

## 概述

可用性（Availability）是系统正常运行时间占总时间的比例，通常以百分比表示。它是系统可靠性和恢复能力的综合指标。

## 核心概念/原理

### 计算公式
A = MTBF / (MTBF + MTTR) × 100%
A = 正常运行时间 / (正常运行时间 + 故障时间) × 100%

## 关键机制或特性

### 可用性等级
| 等级 | 可用性 | 年停机时间 |
|------|--------|----------|
| 99% | 2个9 | 3.65天 |
| 99.9% | 3个9 | 8.77小时 |
| 99.99% | 4个9 | 52.6分钟 |
| 99.999% | 5个9 | 5.26分钟 |

## 使用场景与最佳实践

生产系统通常要求至少 3 个 9（99.9%）的可用性。K8s 通过自愈、多副本和高可用架构来保障可用性。

## 参考链接

- [Availability]()

## Related

- [[domain-10-troubleshooting-diagnostics/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
