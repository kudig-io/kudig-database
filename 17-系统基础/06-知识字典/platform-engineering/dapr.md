---
title: Dapr 分布式应用运行时
description: Dapr（Distributed Application Runtime）是 CNCF 孵化项目，为微服务提供标准化的构建块（Building
  Blocks），...
summary: Dapr（Distributed Application Runtime）是 CNCF 孵化项目，为微服务提供标准化的构建块（Building Blocks），...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- microservices
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dapr 分布式应用运行时 是什么
- Dapr 详解
trigger_keywords:
- Dapr 分布式应用运行时
- Dapr
- dictionary
prerequisites:
- kubernetes
---



# Dapr 分布式应用运行时（Dapr）

## 概述

Dapr（Distributed Application Runtime）是 CNCF 孵化项目，为微服务提供标准化的构建块（Building Blocks），通过 Sidecar 模式抽象服务发现、状态管理、消息发布等分布式系统通用能力。

## 核心概念/原理

- **Sidecar 架构**：以 Sidecar 方式部署，应用通过 HTTP/gRPC 调用 Dapr API
- **构建块模式**：状态管理、服务调用、发布订阅、密钥管理、Actor 等
- **组件可插拔**：每种构建块支持多种后端实现（Redis、Kafka、Azure 等）
- **CNCF 孵化**：微软开源，社区活跃

## 关键机制或特性

- Service Invocation：服务间调用（mTLS + 重试 + 追踪）
- State Management：KV 状态存储抽象
- Pub/Sub：消息发布订阅（多 Broker 支持）
- Bindings：外部系统集成（输入/输出绑定）
- Actors：虚拟 Actor 模型
- Workflow：持久化工作流引擎
- Configuration API 和 Secret Store

## 使用场景与最佳实践

- 微服务应用的标准化运行时
- 多云/混合云的应用可移植性
- 事件驱动的微服务架构
- 状态管理和服务编排
- .NET/Java/Go/Python 等多语言微服务

## 参考链接

- https://dapr.io/
- https://github.com/dapr/dapr

## Related

- [[17-系统基础/06-知识字典/platform-engineering/nats.md|NATS]]
- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/specialized-workloads/knative.md|Knative]]
