---
title: 或门
description: 或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。...
summary: 或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- orgate
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
- 或门 是什么
- OR Gate 详解
trigger_keywords:
- 或门
- OR Gate
- fta
prerequisites:
- troubleshooting-methodology
---



# 或门

> **英文名**: OR Gate

## 概述

或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。

## 核心概念/原理

### 逻辑含义
输出 = 输入1 OR 输入2 OR ... OR 输入N
任一输入发生，输出就发生。所有输入都不发生，输出才不发生。

## 关键机制或特性

或门使故障概率增大（P = 1 - ∏(1-Pi)），是系统脆弱性的标志。多个独立故障路径通过或门汇聚意味着系统缺乏冗余。

## 使用场景与最佳实践

在 K8s 中，Service 不可用可能是因为所有后端 Pod 不可用 OR 网络不通 OR DNS 解析失败。

## 概率计算

或门输出概率为所有输入概率的并集：

```
P(OR) = 1 - (1-P1) × (1-P2) × ... × (1-Pn)
简化（小概率）: P(OR) ≈ P1 + P2 + ... + Pn

示例: Service 不可用
  P(Pod全部崩溃) = 0.01
  P(网络不通) = 0.02
  P(DNS失败) = 0.005
  P(服务不可用) ≈ 0.01 + 0.02 + 0.005 = 0.035
```

或门增加顶事件概率，是故障传播的主要路径。

## K8s 生产案例

### 案例 1: Pod CrashLoopBackOff 故障树

```
顶事件: Pod CrashLoopBackOff
    │
   [OR]
    │
    ├── 基本事件 1: 应用启动失败 (P=0.3)
    │    ├── 配置错误
    │    ├── 依赖服务不可达
    │    └── 资源不足 (OOM)
    │
    ├── 基本事件 2: 镜像问题 (P=0.2)
    │    ├── 镜像不存在
    │    ├── 入口点错误
    │    └── 权限不足
    │
    ├── 基本事件 3: 探针配置错误 (P=0.25)
    │    ├── livenessProbe 路径错误
    │    └── 超时时间过短
    │
    └── 基本事件 4: 资源限制 (P=0.15)
         ├── CPU limit 过低 (throttling)
         └── Memory limit 过低 (OOM Kill)

P(CrashLoop) ≈ 0.3 + 0.2 + 0.25 + 0.15 = 0.9
→ 说明 CrashLoop 是高频故障，需重点排查
```

### 案例 2: 节点 NotReady 故障树

```
顶事件: Node NotReady
    │
   [OR]
    ├── kubelet 进程崩溃 (P=0.1)
    ├── 容器运行时故障 (P=0.15)
    ├── 网络分区 (P=0.2)
    ├── 磁盘压力 (P=0.25)
    ├── 内存压力 (P=0.2)
    └── 证书过期 (P=0.1)
```

## 故障树绘制规范

```
    ┌─────────┐
    │ 输出事件 │
    └────┬────┘
         │
      ┌──┴──┐
      │ OR  │  ← 或门符号
      └──┬──┘
     ┌───┼───┐
     │   │   │
    E1  E2  E3  ← 任一发生即触发
```

## 面试要点

1. **或门在故障树中的作用？**
   - 表示故障传播路径：任一子故障都会导致父故障
   - 概率相加，增加顶事件发生概率
   - 是故障树中最常见的逻辑门

2. **如何减少或门的影响？**
   - 减少或门输入数量（消除单点故障）
   - 降低每个输入事件的概率（提高可靠性）
   - 将或门转换为与门（增加冗余）

3. **K8s 中哪些是典型的或门场景？**
   - Pod 失败原因：配置/镜像/资源/探针（任一导致失败）
   - 节点 NotReady：kubelet/运行时/网络/磁盘（任一导致）
   - API 请求失败：认证/授权/准入/存储（任一导致）

## 参考链接

- [OR Gate]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
