---
title: Database & Middleware
description: '| 05-operator-management/ | 数据库 Operator 设计模式与对比 |'
summary: '| 05-operator-management/ | 数据库 Operator 设计模式与对比 |'
category: domain
tags:
- database
- middleware
- message-queue
- time-series
- operator
- streaming
- prometheus
- flux
- rag
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Database & Middleware 是什么
- 如何 Database & Middleware
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- Database
- Middleware
- database
- middleware
prerequisites:
- kubectl-basics
- prometheus-basics
- gpu-scheduling-basics
---



# Database & Middleware

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-databases/ | 关系型/NoSQL 数据库 |
| 02-middleware/ | 缓存、RPC、网关等中间件 |
| 03-message-queues/ | [[NATS|NATS]]、Pulsar、消息队列选型 |
| 04-time-series-db/ | [[Prometheus|Prometheus]] TSDB、InfluxDB、TimescaleDB |
| 05-operator-management/ | 数据库 Operator 设计模式与对比 |
| 06-data-streaming/ | CDC、流处理框架 |
| 98-merged-indexes/ | 合并索引保留 |

## 与其他 Domain 的关系

- [[domain-04-storage-data/README.md|domain-04-storage-data]] — 存储基础
- [[domain-14-ai-ml-infra/README.md|domain-14-ai-ml-infra]] — AI 数据管道

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic KUDIG Database — Global MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
