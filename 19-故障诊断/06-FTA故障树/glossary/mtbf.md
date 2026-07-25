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

## K8s 组件 MTBF 参考值

| 组件 | 典型 MTBF | 影响因素 |
|------|----------|----------|
| 云 VM 节点 | 1-2 年 | 硬件故障、内核崩溃 |
| etcd 集群 | 3-5 年 | 磁盘寿命、网络稳定性 |
| Pod (无状态) | 30-90 天 | 部署频率、资源压力 |
| 证书 | 1 年 | 默认有效期，可配置 |
| 云盘 (EBS) | 5-10 年 | 云厂商 SLA |

## MTBF 与可用性计算

```
Availability = MTBF / (MTBF + MTTR)

示例: K8s 控制平面
  MTBF = 8760h (1年)
  MTTR = 1h
  Availability = 8760 / (8760 + 1) = 99.989%

目标 99.99% (四个九):
  年停机 < 52.6 min
  需要: MTBF/MTTR > 10000
  即: MTTR < 5min (当 MTBF=1年)
```

## 提高 MTBF 的实践

1. **冗余设计**: 多副本、多 AZ、多区域
2. **预防性维护**: 定期更新证书、升级组件
3. **混沌工程**: 主动发现潜在故障点
4. **资源监控**: 提前发现磁盘/内存/CPU 压力
5. **变更管理**: 严格的发布流程减少人为故障

## 面试要点

1. **MTBF 和 MTTR 如何影响可用性？**
   - Availability = MTBF / (MTBF + MTTR)
   - 提高 MTBF 或降低 MTTR 都能提高可用性
   - 降低 MTTR 通常更实际（MTBF 受硬件限制）

2. **K8s 中如何提高 MTBF？**
   - 多副本 + PDB（防止同时故障）
   - 反亲和性（分散故障域）
   - 定期维护和更新

3. **MTBF 在故障树分析中的作用？**
   - 用于计算基本事件概率：P = 1/MTBF
   - 帮助识别最薄弱环节（最低 MTBF）
   - 指导冗余设计决策

## 参考链接

- [MTBF (Mean Time Between Failures)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
