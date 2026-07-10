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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Database & Middleware

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-databases/ | 关系型/NoSQL 数据库（MySQL、PostgreSQL、Redis、MongoDB、Etcd） |
| 02-cache/ | 缓存中间件（Redis Cluster、Sentinel） |
| 03-message-queues/ | NATS、Pulsar、Kafka 消息队列选型与运维 |
| 04-time-series-db/ | [[Prometheus|Prometheus]] TSDB、InfluxDB、TimescaleDB |
| 05-operator-management/ | 数据库 Operator 设计模式与对比 |
| 06-data-streaming/ | CDC、流处理框架 |
| 98-merged-indexes/ | 合并索引保留 |

## 与其他 Domain 的关系

- [[存储/README.md|存储]] — 存储基础
- [[AI基础设施/README.md|AI基础设施]] — AI 数据管道

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- 网络 KUDIG Database — Global MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
