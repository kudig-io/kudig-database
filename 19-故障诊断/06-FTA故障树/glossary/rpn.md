---
title: 风险优先级数
description: RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。...
summary: RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- rpn
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
- 风险优先级数 是什么
- RPN (Risk Priority Number) 详解
trigger_keywords:
- 风险优先级数
- RPN (Risk Priority Number)
- fta
prerequisites:
- troubleshooting-methodology
---



# 风险优先级数

> **英文名**: RPN (Risk Priority Number)

## 概述

RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。

## 核心概念/原理

### 计算公式
RPN = S × O × D
- S (Severity)：严重度（1-10，10 最严重）
- O (Occurrence)：发生频率（1-10，10 最频繁）
- D (Detection)：可检测性（1-10，10 最难检测）
RPN 范围：1-1000

## 关键机制或特性

RPN 用于对故障模式进行优先级排序。高 RPN 值的故障模式应优先处理。但需注意：即使 RPN 中等，严重度极高的故障也应优先处理。

## 使用场景与最佳实践

在 K8s 运维中，用 RPN 评估不同故障场景的风险等级，指导应急预案和资源投入。

## K8s 故障场景 RPN 评估

| 故障场景 | S | O | D | RPN | 优先级 |
|---------|---|---|---|-----|--------|
| CNI 配置错误导致网络中断 | 9 | 3 | 5 | 135 | P0 |
| 证书过期导致 API 不可用 | 9 | 4 | 2 | 72 | P1 |
| etcd 磁盘故障 | 10 | 2 | 3 | 60 | P1 |
| Pod OOM Kill | 6 | 7 | 2 | 84 | P1 |
| DNS 解析延迟 | 5 | 5 | 4 | 100 | P1 |
| 节点磁盘压力 | 7 | 4 | 3 | 84 | P1 |
| HPA 未生效 | 5 | 3 | 3 | 45 | P2 |

## RPN 决策矩阵

```
RPN > 100: 立即处理 (P0)
  → 制定应急预案、自动化修复

RPN 50-100: 计划处理 (P1)
  → 纳入迭代计划、增加监控

RPN < 50: 观察 (P2)
  → 定期审查、接受风险

注意: 即使 RPN 中等，S=10 的故障也应优先处理
  例: etcd 数据丢失 (S=10, O=1, D=5, RPN=50)
  虽然 RPN 只有 50，但后果不可接受
```

## 降低 RPN 的策略

| 维度 | 策略 | K8s 实践 |
|------|------|----------|
| 降低 S | 冗余设计 | 多副本、多 AZ |
| 降低 O | 预防维护 | 自动证书轮换、定期更新 |
| 降低 D | 增强监控 | Prometheus 告警、日志聚合 |

## 面试要点

1. **RPN 的三个组成部分？**
   - 严重度(S) × 发生度(O) × 检测度(D)
   - 每个维度 1-10 分
   - RPN 范围 1-1000

2. **RPN 的局限性？**
   - 相同 RPN 可能有不同风险特征
   - S=10 的故障即使 RPN 低也应优先
   - 需要结合业务上下文判断

3. **如何用 RPN 指导 K8s 运维投入？**
   - 高 RPN: 自动化修复 + 应急预案
   - 中 RPN: 增强监控 + 纳入计划
   - 低 RPN: 接受风险 + 定期审查

## 参考链接

- [RPN (Risk Priority Number)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
