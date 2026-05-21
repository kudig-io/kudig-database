---
title: 消息队列选型对比
description: '| **分层存储** | 有限 | ✅ | ❌ | ❌ |'
category: domain
tags:
- message-queue
- kafka
- pulsar
- nats
- rabbitmq
- comparison
- helm
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 消息队列选型对比 是什么
- 如何 消息队列选型对比
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- 消息队列选型对比
- database
- middleware
prerequisites:
- kubectl-basics
- helm-basics
- kafka-basics
---

# 消息队列选型对比

## 综合对比

| 维度 | Kafka | Pulsar | NATS (JetStream) | RabbitMQ |
|------|-------|--------|------------------|----------|
| **吞吐量** | 极高 | 极高 | 高 | 中 |
| **延迟** | 低(ms) | 低(ms) | 极低(µs) | 低(ms) |
| **持久化** | ✅ | ✅ | ✅ | ✅ |
| **多租户** | ❌ | ✅ | ❌ | ✅ |
| **Geo-Repl** | MirrorMaker | 原生 | ❌ | Shovel/Federation |
| **分层存储** | 有限 | ✅ | ❌ | ❌ |
| **K8s 适配** | Operator | Operator | Helm | Operator |
| **运维复杂度** | 高 | 中 | 低 | 中 |
| **社区** | 极大 | 大 | 中 | 大 |

## 选型决策树

```
需求?
├── 极高吞吐 + 成熟生态 → Kafka
├── 计算存储分离 + 多租户 → Pulsar
├── 极简运维 + 低延迟 → NATS
├── 复杂路由 + 企业集成 → RabbitMQ
└── 云原生 + 轻量 → NATS / Pulsar
```

## 相关

- [[domain-16-database-middleware/03-message-queues/01-nats-deep-dive]]
- [[domain-16-database-middleware/03-message-queues/02-pulsar-architecture]]
