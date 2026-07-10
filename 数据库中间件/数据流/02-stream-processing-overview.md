---
title: 流处理技术概述
description: '# 流处理技术概述'
summary: '# 流处理技术概述'
category: domain
tags:
- stream-processing
- flink
- spark-streaming
- kafka-streams
- data-engineering
- kafka
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[数据库中间件/数据流/01-cdc-change-data-capture.md|01 cdc change data capture]]


<!-- risk-assessed -->
