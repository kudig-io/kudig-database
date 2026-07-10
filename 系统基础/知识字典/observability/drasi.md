---
title: Drasi 变更检测
description: Drasi 是微软开源的 CNCF Sandbox 项目，实时检测基础设施和应用状态的变化，通过连续查询（Continuous Query）监控数据变化并触发反...
summary: Drasi 是微软开源的 CNCF Sandbox 项目，实时检测基础设施和应用状态的变化，通过连续查询（Continuous Query）监控数据变化并触发反...
category: dictionary
tags:
- k8s
- glossary
- observability
- change-detection
- microsoft
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Drasi 变更检测 是什么
- Drasi 详解
trigger_keywords:
- Drasi 变更检测
- Drasi
- dictionary
prerequisites:
- kubernetes
---



# Drasi 变更检测（Drasi）

## 概述

Drasi 是微软开源的 CNCF Sandbox 项目，实时检测基础设施和应用状态的变化，通过连续查询（Continuous Query）监控数据变化并触发反应。

## 核心概念/原理

- **变更检测**：实时监控数据状态的变化
- **连续查询**：基于 Cypher 的连续查询引擎
- **CNCF Sandbox**：微软主导
- **事件驱动**：变化触发自动化反应

## 关键机制或特性

- Source 定义数据源（K8s/Gremlin/PostgreSQL）
- ContinuousQuery 定义监控条件
- Reaction 定义变化响应
- 基于 Cypher 图查询语言
- 状态变化追踪（Added/Updated/Deleted）
- Kubernetes 资源变化监控
- Webhook/Log/Teams 反应

## 使用场景与最佳实践

- 基础设施变更的实时监控
- K8s 资源状态变化的告警
- 应用配置的漂移检测
- 安全事件的实时响应
- 运维自动化的事件触发

## 参考链接

- https://drasi.dev/
- https://github.com/drasI-project/drasI

## Related

- [[系统基础/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[系统基础/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]
- [[系统基础/topic-dictionary/operations/kuberhealthy.md|Kuberhealthy]]
