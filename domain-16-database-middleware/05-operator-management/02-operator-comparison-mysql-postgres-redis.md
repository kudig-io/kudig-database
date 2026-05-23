---
title: MySQL PostgreSQL Redis Operator 对比
description: '| **监控** | MySQL Exporter | PostgreSQL Exporter | Redis Exporter |'
category: domain
tags:
- kubernetes
- operator
- mysql
- postgresql
- redis
- comparison
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MySQL PostgreSQL Redis Operator 对比 是什么
- 如何 MySQL PostgreSQL Redis Operator 对比
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- MySQL
- PostgreSQL
- Redis
- Operator
- 对比
- database
- middleware
prerequisites:
- kubectl-basics
- redis-basics
- mysql-basics
created: "2026-05-23"
---

# MySQL PostgreSQL Redis Operator 对比

## 功能对比

| 功能 | MySQL Operator | PostgreSQL ([[CloudNativePG|CloudNativePG]]) | Redis Operator |
|------|---------------|---------------------------|----------------|
| **高可用** | Group Replication | Streaming Replication | Sentinel / Cluster |
| **故障转移** | 自动 | 自动 | 自动 |
| **备份** | 物理/逻辑 | 物理 (WAL + base) | RDB/AOF |
| **恢复** | 时间点恢复 | 时间点恢复 | 全量恢复 |
| **监控** | MySQL Exporter | PostgreSQL Exporter | Redis Exporter |
| **升级** | 滚动升级 | 滚动升级 | 滚动升级 |
| **连接池** | Router | PgBouncer (Sidecar) | 原生 |

## 相关

- [[domain-16-database-middleware/05-operator-management/01-database-operator-patterns]]
