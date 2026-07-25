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

## K8s 可用性等级与停机时间

| 可用性 | 年停机 | 月停机 | K8s 场景 |
|--------|--------|--------|----------|
| 99% (两个九) | 3.65天 | 7.3h | 开发环境 |
| 99.9% (三个九) | 8.76h | 43.8min | 内部服务 |
| 99.99% (四个九) | 52.6min | 4.38min | 生产服务 |
| 99.999% (五个九) | 5.26min | 26.3s | 金融/电信 |

## K8s 可用性计算

```
单组件可用性:
  Availability = MTBF / (MTBF + MTTR)

串联系统 (所有组件必须可用):
  A_total = A1 × A2 × ... × An
  例: apiserver(99.99%) × etcd(99.99%) × 网络(99.9%)
     = 99.88% (串联降低可用性)

并联系统 (任一可用即可):
  A_total = 1 - (1-A1) × (1-A2) × ... × (1-An)
  例: 3个 apiserver 副本 (各 99.9%)
     = 1 - 0.001³ = 99.9999999% (并联提高可用性)
```

## 提高 K8s 可用性的实践

1. **控制平面 HA**: apiserver 多副本 + etcd 3/5 节点
2. **工作负载冗余**: 多副本 + PDB + 反亲和
3. **多 AZ 部署**: 跨可用区分布 Pod
4. **自动恢复**: 健康检查 + 自动重启 + 自愈控制器
5. **降级策略**: 熔断 + 限流 + 降级响应

## 面试要点

1. **如何计算 K8s 服务的可用性？**
   - 串联：所有组件可用性相乘
   - 并联：1 - 所有组件不可用性相乘
   - 关键路径上的组件决定整体可用性

2. **99.99% 可用性意味着什么？**
   - 年停机 < 52.6 分钟
   - 需要 MTTR < 5min（假设 MTBF=1年）
   - 需要自动化故障检测和恢复

3. **K8s 中如何实现四个九可用性？**
   - 控制平面 HA（多副本 + 多 AZ）
   - 工作负载多副本 + PDB
   - 自动化故障转移（< 5min）
   - 定期混沌工程演练

## 参考链接

- [Availability]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
