---
title: 消息队列选型对比
description: '| **分层存储** | 有限 | ✅ | ❌ | ❌ |'
summary: '| **分层存储** | 有限 | ✅ | ❌ | ❌ |'
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
tier: supporting
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 消息队列选型对比

## 综合对比

| 维度 | Kafka | Pulsar | [[NATS|NATS]] (JetStream) | RabbitMQ |
|------|-------|--------|------------------|----------|
| **吞吐量** | 极高 | 极高 | 高 | 中 |
| **延迟** | 低(ms) | 低(ms) | 极低(µs) | 低(ms) |
| **持久化** | ✅ | ✅ | ✅ | ✅ |
| **多租户** | ❌ | ✅ | ❌ | ✅ |
| **Geo-Repl** | MirrorMaker | 原生 | ❌ | Shovel/Federation |
| **分层存储** | 有限 | ✅ | ❌ | ❌ |
| **K8s 适配** | Operator | Operator | [[Helm|Helm]] | Operator |
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

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-16-database-middleware/04-message-queues/01-nats-deep-dive|01 nats deep dive]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-16-database-middleware/04-message-queues/02-pulsar-architecture|02 pulsar architecture]]


<!-- risk-assessed -->
