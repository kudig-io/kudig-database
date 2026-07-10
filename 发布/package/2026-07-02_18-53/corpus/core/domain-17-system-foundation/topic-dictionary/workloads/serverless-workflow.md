---
title: Serverless Workflow 编排
description: Serverless Workflow 是 CNCF Sandbox 项目，定义了事件驱动工作流的声明式规范，使用 YAML/JSON 描述工作流逻辑，支持多种...
summary: Serverless Workflow 是 CNCF Sandbox 项目，定义了事件驱动工作流的声明式规范，使用 YAML/JSON 描述工作流逻辑，支持多种...
category: dictionary
tags:
- k8s
- glossary
- workloads
- serverless
- orchestration
tier: core
created: 2026-05
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Serverless Workflow 编排 是什么
- Serverless Workflow 详解
trigger_keywords:
- Serverless Workflow 编排
- Serverless Workflow
- dictionary
prerequisites:
- kubernetes
---



# Serverless Workflow 编排（Serverless Workflow）

## 概述

Serverless Workflow 是 CNCF Sandbox 项目，定义了事件驱动工作流的声明式规范，使用 YAML/JSON 描述工作流逻辑，支持多种 Serverless 平台的执行。

## 核心概念/原理

- **工作流标准**：定义事件驱动工作流的通用规范
- **声明式**：YAML/JSON 描述工作流状态和转换
- **CNCF Sandbox**：厂商中立的编排标准
- **多平台**：可在 Knative/Apache Kogito/Azure 等平台执行

## 关键机制或特性

- State/Transition 工作流模型
- Event 触发和过滤
- Action 执行（函数调用/事件发送/子流程）
- Parallel/Foreach/Switch 控制流
- Error/Retry/Timeout 处理
- Compensation 补偿事务
- SDK（Go/Java/TypeScript）

## 使用场景与最佳实践

- Serverless 应用的业务流程编排
- 微服务间的复杂工作流协调
- 事件驱动架构的流程管理
- 长事务的 Saga 模式实现
- 多云工作流的可移植定义

## 参考链接

- https://serverlessworkflow.io/
- https://github.com/serverlessworkflow/specification

## Related

- [[domain-17-system-foundation/知识字典/specialized-workloads/knative.md|Knative]]
- [[domain-17-system-foundation/知识字典/platform-engineering/dapr.md|Dapr]]
- [[domain-17-system-foundation/知识字典/platform-engineering/nats.md|NATS]]
