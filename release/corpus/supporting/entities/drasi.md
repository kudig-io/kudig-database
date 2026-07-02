---
title: Drasi (entities)
description: '## 概述'
summary: 'Drasi 是由 Microsoft 开发的数据变更处理平台，允许你持续检测数据源中的变更并自动做出反应。它使用 Continuous Query（持续查询）对来自数据库、消息队列、事件流等多种数据源的变更进行实时过滤、聚合和关联，当查询结果发生变化时触发下游动作（如发送通知、调用 API、更新其他系统）。'
category: entities
tags:
- k8s
- cncf
- streaming
- drasi
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Drasi 是什么
- 如何 Drasi
trigger_keywords:
- Drasi
prerequisites:
- kubectl-basics
---



# Drasi

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Rust, C#

## 概述

Drasi 是由 Microsoft 开发的数据变更处理平台，允许你持续检测数据源中的变更并自动做出反应。它使用 Continuous Query（持续查询）对来自数据库、消息队列、事件流等多种数据源的变更进行实时过滤、聚合和关联，当查询结果发生变化时触发下游动作（如发送通知、调用 API、更新其他系统）。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **查询优化**: 在 WHERE 子句中尽早过滤，减少需要维护的结果集大小
- **数据源范围**: 只订阅查询实际需要的表/资源，减少变更事件处理量
- **幂等反应**: 设计反应器处理逻辑时保证幂等性，应对重复触发
- **监控查询**: 监控持续查询的延迟和吞吐，及时发现性能瓶颈
- **渐进部署**: 先用 Debug 反应器验证查询逻辑正确，再切换到生产反应器

## 架构定位

在 CNCF 生态中，drasi 属于 **Streaming** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[youki]] — youki
- [[easegress]] — Easegress
- [[perses]] — Perses
- [[tremor]] — Tremor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- drasi
- [[entities/nats.md|[[NATS|NATS]]]]
- [[entities/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
