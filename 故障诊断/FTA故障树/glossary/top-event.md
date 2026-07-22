---
title: 顶事件
description: 顶事件（Top Event）是故障树最顶层的不期望事件，是整个故障树分析的起点。它代表了需要分析的系统级故障或异常状态。...
summary: 顶事件（Top Event）是故障树最顶层的不期望事件，是整个故障树分析的起点。它代表了需要分析的系统级故障或异常状态。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- topevent
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
- 顶事件 是什么
- Top Event 详解
trigger_keywords:
- 顶事件
- Top Event
- fta
prerequisites:
- troubleshooting-methodology
---



# 顶事件

> **英文名**: Top Event

## 概述

顶事件（Top Event）是故障树最顶层的不期望事件，是整个故障树分析的起点。它代表了需要分析的系统级故障或异常状态。

## 核心概念/原理

### 在 K8s 中的示例
- Pod 处于 CrashLoopBackOff 状态
- 集群 API Server 不可用
- Service 无法访问
- 节点 NotReady

## 关键机制或特性

顶事件必须是可观察的、明确的、可验证的。它定义了分析的边界和目标。

## 使用场景与最佳实践

在 FTA 诊断中，顶事件通常来自告警、用户反馈或监控系统检测到的异常。

## K8s 常见顶事件

| 顶事件 | 来源 | 影响级别 |
|--------|------|----------|
| Service 不可用 | 告警/用户反馈 | P0 紧急 |
| API 延迟 > 1s | Prometheus 告警 | P1 高 |
| Pod CrashLoopBackOff | K8s 事件 | P1 高 |
| 节点 NotReady | 节点监控 | P1 高 |
| etcd 延迟 > 100ms | etcd 监控 | P2 中 |
| 证书即将过期 | 定期扫描 | P3 低 |
| 磁盘使用率 > 85% | 节点监控 | P2 中 |

## 顶事件定义规范

```
顶事件定义模板:
  1. 明确故障现象: "什么不工作了"
  2. 量化指标: "延迟 > Xms" / "错误率 > Y%"
  3. 影响范围: "哪些用户/服务受影响"
  4. 时间窗口: "持续多久"

示例:
  ✗ "服务有问题" (模糊)
  ✓ "生产环境 API 服务 P99 延迟 > 2s 持续 5min" (明确)
```

## 从顶事件到故障树

```
顶事件: 生产 API 服务不可用
    │
   [OR]
    ├── 所有 Pod 崩溃
    │    [OR]
    │    ├── OOM Kill
    │    ├── 配置错误
    │    └── 依赖服务不可用
    │
    ├── 网络不可达
    │    [OR]
    │    ├── DNS 解析失败
    │    ├── CNI 故障
    │    └── 防火墙规则变更
    │
    └── 流量过载
         [OR]
         ├── DDoS 攻击
         ├── 上游突发流量
         └── HPA 未生效
```

## 面试要点

1. **如何定义一个好的顶事件？**
   - 明确、可量化、可观测
   - 包含影响范围和时间窗口
   - 来自实际告警或用户反馈

2. **顶事件和告警的关系？**
   - 告警是顶事件的检测机制
   - 一个顶事件可能对应多个告警规则
   - 告警质量直接影响 MTTD

3. **K8s 中如何确定顶事件优先级？**
   - P0: 影响所有用户（服务完全不可用）
   - P1: 影响部分用户（功能降级）
   - P2: 潜在风险（尚未影响用户）

## 参考链接

- [Top Event]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
