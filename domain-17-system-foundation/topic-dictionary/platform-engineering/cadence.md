---
title: Cadence 工作流引擎
description: Cadence 是 Uber 开源的分布式工作流引擎（后由 Uber 团队成立独立公司维护），为长时间运行的有状态应用提供持久化执行、重试和可见性能力。...
summary: Cadence 是 Uber 开源的分布式工作流引擎（后由 Uber 团队成立独立公司维护），为长时间运行的有状态应用提供持久化执行、重试和可见性能力。...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- workflow
- uber
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cadence 工作流引擎 是什么
- Cadence 详解
trigger_keywords:
- Cadence 工作流引擎
- Cadence
- dictionary
prerequisites:
- kubernetes
---



# Cadence 工作流引擎（Cadence）

## 概述

Cadence 是 Uber 开源的分布式工作流引擎（后由 Uber 团队成立独立公司维护），为长时间运行的有状态应用提供持久化执行、重试和可见性能力。

## 核心概念/原理

- **持久化工作流**：工作流状态持久化，崩溃后自动恢复
- **长时间运行**：支持数月甚至数年的工作流
- **Uber 开源**：经过 Uber 大规模生产验证
- **Temporal 前身**：Temporal 是 Cadence 的演进版本

## 关键机制或特性

- Workflow/Activity 编程模型
- 信号（Signal）和查询（Query）
- 定时器（Timer）和子工作流
- 版本管理和迁移
- 搜索属性（Search Attributes）
- Cadence Web UI
- 多租户 Domain 管理

## 使用场景与最佳实践

- 长时间运行的业务流程编排
- 微服务的分布式事务协调
- 基础设施自动化工作流
- 数据 Pipeline 编排
- 定时任务和 Cron 替代

## 参考链接

- https://cadenceworkflow.io/
- https://github.com/cadence-workflow/cadence

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/dapr.md|Dapr]]
- [[domain-17-system-foundation/topic-dictionary/workloads/serverless-workflow.md|Serverless Workflow]]
- [[domain-17-system-foundation/topic-dictionary/operations/tekton.md|Tekton]]
