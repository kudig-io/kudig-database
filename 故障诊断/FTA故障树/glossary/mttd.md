---
title: 平均检测时间
description: MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。...
summary: MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- mttd
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
- 平均检测时间 是什么
- MTTD (Mean Time To Detect) 详解
trigger_keywords:
- 平均检测时间
- MTTD (Mean Time To Detect)
- fta
prerequisites:
- troubleshooting-methodology
---



# 平均检测时间

> **英文名**: MTTD (Mean Time To Detect)

## 概述

MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。

## 核心概念/原理

### 影响因素
- 监控覆盖度：未监控的组件故障无法被检测。
- 告警灵敏度：阈值过高导致延迟检测。
- 检测手段：主动探测 vs 被动告警。

## 关键机制或特性

通过完善监控覆盖、优化告警阈值和引入主动健康检查可以缩短 MTTD。

## 使用场景与最佳实践

在 K8s 中，使用 Prometheus 告警、Liveness/Readiness Probe 和 SLO 监控来缩短 MTTD。

## 参考链接

- [MTTD (Mean Time To Detect)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
