---
title: CDC 变更数据捕获
description: │   → 数据库触发器写变更日志表
category: domain
tags:
- cdc
- change-data-capture
- data-streaming
- debezium
- kafka-connect
- mysql
- kafka
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CDC 变更数据捕获 是什么
- 如何 CDC 变更数据捕获
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- CDC
- 变更数据捕获
- database
- middleware
prerequisites:
- kubectl-basics
- kafka-basics
- mysql-basics
created: "2026-05-23"
---

# CDC 变更数据捕获

## CDC 模式

```
CDC 三种实现方式:
├── 基于查询 (Query-based)
│   → 定时 SELECT * WHERE updated_at > ?
│   → 简单但性能差，无法捕获 DELETE
├── 基于触发器 (Trigger-based)
│   → 数据库触发器写变更日志表
│   → 影响写入性能
└── 基于日志 (Log-based) ⭐推荐
    → 读取数据库 WAL / binlog
    → 无性能影响，完整捕获
```

## Debezium 架构

```
┌──────────┐     ┌─────────────┐     ┌──────────┐
│  MySQL   │────→│  Debezium   │────→│  Kafka   │
│ (binlog) │     │  Connector  │     │ (Topic)  │
└──────────┘     └─────────────┘     └──────────┘
```

## [[Kubernetes|Kubernetes]] 部署

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaConnect
metadata:
  name: debezium-connect
spec:
  replicas: 1
  bootstrapServers: kafka:9092
  config:
    config.storage.replication.factor: 3
    offset.storage.replication.factor: 3
    status.storage.replication.factor: 3
```

## 相关

- [[domain-16-database-middleware/06-data-streaming/02-stream-processing-overview.md|02 stream processing overview]]
