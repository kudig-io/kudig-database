---
title: Apache Pulsar 架构解析
description: '# Apache Pulsar 架构解析'
category: domain
tags:
- pulsar
- message-queue
- streaming
- tiered-storage
- kafka
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Apache Pulsar 架构解析 是什么
- 如何 Apache Pulsar 架构解析
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- Apache
- Pulsar
- 架构解析
- database
- middleware
prerequisites:
- kubectl-basics
- kafka-basics
---

# Apache Pulsar 架构解析

## 分层架构

```
Pulsar 架构:
┌─────────────────────────────────────┐
│           Pulsar Broker             │  ← 计算层（无状态）
│  (Topic Lookup, Producer, Consumer) │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         Apache BookKeeper           │  ← 存储层（持久化）
│      (Ledger, Entry, Journal)       │
└─────────────────────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         Apache ZooKeeper            │  ← 元数据层
│      (Configuration, Coordination)  │
└─────────────────────────────────────┘
```

## 计算-存储分离的优势

```
独立扩缩容:
  读压力大 → 增加 Broker（秒级）
  写压力大 → 增加 Bookie（分钟级）
  存储满 → 增加磁盘或启用分层存储

对比 Kafka:
  Kafka: Broker 绑定存储，扩容需数据重平衡
  Pulsar: Broker 无状态，随时增删
```

## 多租户与命名空间

```
Tenant → Namespace → Topic

权限控制:
  Tenant 级别: 身份认证、角色分配
  Namespace 级别: 配额、策略、隔离
```

## Geo-Replication

```
同步复制: 写本地 + 同步写远端（高延迟，强一致）
异步复制: 写本地 + 后台复制（低延迟，最终一致）
```

## 相关

- [[domain-16-database-middleware/03-message-queues/01-nats-deep-dive]]
- [[domain-16-database-middleware/02-middleware/01-kafka-deep-dive]]
