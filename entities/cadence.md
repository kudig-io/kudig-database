---
title: Cadence (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- streaming
- cadence
- mysql
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cadence 是什么
- 如何 Cadence
trigger_keywords:
- Cadence
prerequisites:
- kubectl-basics
- mysql-basics
---



# Cadence

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Go

## 概述

Cadence 是一个分布式、可扩展、持久化的工作流编排引擎，用于以可靠、可扩展的方式执行异步长时间运行的业务逻辑。Cadence 由 Uber 开源，能将复杂的分布式系统交互逻辑简化为简单的编程模型，自动处理失败重试、状态持久化和超时管理。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **幂等 Activity**: 所有 Activity 实现幂等性，确保重试安全
- **工作流版本化**: 使用 `workflow.GetVersion()` 安全升级工作流定义
- **超时配置**: 为每个 Activity 配置合理的超时和重试策略
- **补偿逻辑**: 关键业务流程实现 Saga 补偿模式
- **监控**: 监控工作流执行延迟、Activity 失败率和 Task List 积压
- **持久化选择**: 高吞吐使用 Cassandra，低延迟使用 MySQL

## 架构定位

在 CNCF 生态中，cadence 属于 **Streaming** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[krkn]] — Krkn
- [[opengitops]] — OpenGitOps
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cadence
- [[entities/drasi.md|[[Drasi|Drasi]]]]
- [[entities/tremor.md|[[Tremor|Tremor]]]]
- [[entities/nats.md|NATS]]
- [[entities/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
