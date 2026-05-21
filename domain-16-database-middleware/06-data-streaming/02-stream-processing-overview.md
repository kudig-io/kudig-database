---
title: 流处理技术概述
description: '# 流处理技术概述'
category: domain
tags:
- stream-processing
- flink
- spark-streaming
- kafka-streams
- data-engineering
- kafka
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 流处理技术概述 是什么
- 如何 流处理技术概述
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- 流处理技术概述
- database
- middleware
prerequisites:
- kubectl-basics
- kafka-basics
---

# 流处理技术概述

## 框架对比

| 特性 | Apache Flink | Spark Streaming | Kafka Streams |
|------|-------------|-----------------|---------------|
| **处理语义** | Exactly-Once | Exactly-Once | At-Least-Once / Exactly-Once |
| **延迟** | 毫秒级 | 秒级（微批） | 毫秒级 |
| **状态管理** | 内置 State Backend | RDD Checkpoint | Kafka Topic |
| **SQL 支持** | Flink SQL | Spark SQL | KSQL |
| **K8s 原生** | Flink K8s Operator | Spark Operator | 原生集成 |
| **适用场景** | 复杂事件处理 | 批流统一 | 轻量流处理 |

## 选型建议

```
选择 Flink:
  - 复杂事件处理 (CEP)
  - 低延迟要求 (< 1s)
  - 有状态计算

选择 Spark Streaming:
  - 已有 Spark 生态
  - 批流统一处理
  - 复杂机器学习集成

选择 Kafka Streams:
  - 纯 Kafka 生态
  - 轻量处理
  - 无额外基础设施
```

## 相关

- [[domain-16-database-middleware/06-data-streaming/01-cdc-change-data-capture]]
