---
title: NATS 消息系统
description: 'NATS 是 CNCF 孵化项目，高性能的轻量级消息系统，支持 Core Pub/Sub、JetStream 持久化和 Request/Reply 模式，在 I...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- messaging
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NATS 消息系统 是什么
- NATS 详解
trigger_keywords:
- NATS 消息系统
- NATS
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# NATS 消息系统（NATS）

## 概述

NATS 是 CNCF 孵化项目，高性能的轻量级消息系统，支持 Core Pub/Sub、JetStream 持久化和 Request/Reply 模式，在 IoT、边缘计算和微服务场景中广泛使用。

## 核心概念/原理

- **极致轻量**：单二进制，内存占用极低（MB 级）
- **多模式**：Core（Pub/Sub）+ JetStream（持久化流）+ Request/Reply
- **集群原生**：支持 Leaf Node、Super Cluster 等拓扑
- **CNCF 孵化**：活跃的开源消息中间件社区

## 关键机制或特性

- Core NATS：低延迟 Pub/Sub（微秒级）
- JetStream：持久化消息流（类似 Kafka 轻量替代）
- Subject 通配符匹配（`>` 和 `*`）
- 消费者组（Consumer Groups）和工作队列
- 多租户（Account 隔离）
- NKeys 和 JWT 认证

## 使用场景与最佳实践

- 微服务间的轻量级消息传递
- IoT 设备的数据收集和分发
- 边缘计算场景的本地消息总线
- 事件驱动架构的轻量替代方案
- Kubernetes 内部的事件和通知系统

## 参考链接

- https://nats.io/
- https://github.com/nats-io/nats-server

## Related

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative.md|Knative Eventing]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/grpc.md|gRPC]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/dapr.md|Dapr]]
