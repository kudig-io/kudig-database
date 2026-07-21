---
title: Application Sub-Patterns
description: 应用子模式知识域 — 微服务拆分、Event Sourcing/CQRS、Saga 分布式事务、Sidecar 模式、混沌韧性
category: subdomain
tags:
- microservice
- cqrs
- saga
- sidecar
- resilience
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 应用子模式 Sub-Patterns

> 微服务架构中的核心设计模式与分布式事务实践。

## 模式分类

| 类别 | 模式 | 解决问题 |
|------|------|----------|
| 拆分 | Strangler Fig/Branch by Abstraction | 单体迁移微服务 |
| 数据 | Event Sourcing/CQRS | 数据一致性/查询分离 |
| 事务 | Saga (编排/协调) | 分布式事务 |
| 部署 | Sidecar/Ambassador | 关注点分离 |
| 韧性 | Circuit Breaker/Bulkhead | 故障隔离 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[应用模式/子模式/01-microservice-decomposition-strategies.md\|微服务拆分]] | 拆分策略与边界划分 | advanced |
| [[应用模式/子模式/02-event-sourcing-cqrs-patterns.md\|ES/CQRS]] | 事件源与命令查询分离 | advanced |
| [[应用模式/子模式/03-saga-distributed-transaction.md\|Saga 事务]] | 分布式事务编排与协调 | advanced |
| [[应用模式/子模式/04-sidecar-ambassador-patterns.md\|Sidecar 模式]] | 边车/大使模式实践 | intermediate |
| [[应用模式/子模式/05-chaos-resilience-patterns.md\|混沌韧性]] | 韧性设计模式 | advanced |

## Related

- [[应用模式/index.md|应用模式总索引]]
- [[网络/服务网格/index.md|Service Mesh]]
- [[可靠性/index.md|可靠性]]
