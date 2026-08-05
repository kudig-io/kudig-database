---
title: MySQL PostgreSQL Redis Operator 对比
description: '| **监控** | MySQL Exporter | PostgreSQL Exporter | Redis Exporter |'
summary: '| **监控** | MySQL Exporter | PostgreSQL Exporter | Redis Exporter |'
category: domain
tags:
- kubernetes
- operator
- mysql
- postgresql
- redis
- comparison
tier: supporting
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-16-database-middleware/06-operator-management/01-database-operator-patterns|01 database operator patterns]]


<!-- risk-assessed -->
