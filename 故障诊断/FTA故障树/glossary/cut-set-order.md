---
title: 割集阶数
description: 割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。...
summary: 割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- cutsetorder
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
- 割集阶数 是什么
- Cut Set Order 详解
trigger_keywords:
- 割集阶数
- Cut Set Order
- fta
prerequisites:
- troubleshooting-methodology
---



# 割集阶数

> **英文名**: Cut Set Order

## 概述

割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。

## 核心概念/原理

### 风险等级
- **阶数 1**：单点故障（最危险），一个事件就能导致系统故障。
- **阶数 2**：双重故障，两个事件同时发生才导致系统故障。
- **阶数 3+**：需要多个事件同时发生，概率较低。

## 关键机制或特性

消除阶数为 1 的最小割集（单点故障）是提升系统可靠性的首要目标。

## 使用场景与最佳实践

在 K8s 中，API Server 单实例部署是 1 阶割集（单点故障），应通过多副本消除。

## K8s 中的割集阶数分析

```
割集阶数 = 割集中基本事件的数量

K8s 示例: Service 不可用故障树

1阶割集 (单点故障, 最高优先级):
  {证书过期} → 直接导致 API 不可用
  {CoreDNS 崩溃} → 直接导致 DNS 失败

2阶割集 (双重故障):
  {apiserver故障, etcd故障} → 控制平面不可用
  {CNI故障, DNS故障} → 网络完全中断

3阶割集 (三重故障):
  {AZ-1故障, AZ-2故障, AZ-3故障} → 区域不可用

优先级: 1阶 > 2阶 > 3阶
  → 优先消除 1 阶割集（单点故障）
```

## 割集阶数与可靠性

| 阶数 | 含义 | 概率 | K8s 策略 |
|------|------|------|----------|
| 1阶 | 单点故障 | 最高 | 增加冗余 |
| 2阶 | 双重故障 | 中等 | 分散故障域 |
| 3阶+ | 多重故障 | 最低 | 接受风险 |

## 面试要点

1. **割集阶数的意义？**
   - 阶数越低，风险越高
   - 1 阶割集 = 单点故障，必须优先消除
   - 高阶割集概率低，可接受风险

2. **如何降低割集阶数？**
   - 增加冗余：将 1 阶提升为 2 阶
   - 分散故障域：多 AZ、多区域
   - 自动恢复：降低基本事件概率

3. **K8s 中常见的 1 阶割集？**
   - 证书过期（无自动轮换）
   - 单副本 CoreDNS
   - 单节点控制平面
   - 无备份的有状态服务

## 参考链接

- [Cut Set Order]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
