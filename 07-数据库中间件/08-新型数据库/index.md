---
title: 新型数据库与中间件
description: 向量数据库、图数据库、NewSQL、OLAP、连接池、Schema 迁移等新型数据基础设施知识目录
summary: 覆盖 Milvus/Qdrant 向量数据库、Neo4j/NebulaGraph 图数据库、CockroachDB/YugabyteDB NewSQL、StarRocks/Doris OLAP、PgBouncer/ProxySQL 连接池、Flyway/gh-ost Schema 迁移、PMM 可观测性、pgBackRest 备份、DataHub 数据目录、ShardingSphere/Vitess 分片
category: 数据库中间件
tags:
- database
- vector-database
- newsql
- olap
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 5min
intent_queries:
- "新型数据库有哪些"
- "数据库中间件知识目录"
trigger_keywords:
- 向量数据库
- NewSQL
- OLAP
- 分库分表
prerequisites:
- kubectl-basics
- database-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 新型数据库与中间件

本目录收录 Kubernetes 上新型数据库与数据中间件的生产运维知识，覆盖从 AI 向量检索到分布式 SQL、从 OLAP 分析到数据治理的完整数据基础设施栈。每篇文章均包含架构解析、生产部署 YAML、运维操作命令和故障排查指南。

## 知识地图

### AI 数据基础设施

| 文件 | 主题 | 核心技术 |
|------|------|---------|
| [[07-数据库中间件/08-新型数据库/01-vector-database-milvus-weaviate-qdrant.md]] | 向量数据库 | Milvus / Weaviate / Qdrant / pgvector / Chroma，HNSW/IVF 索引，内存规划 |
| [[07-数据库中间件/08-新型数据库/02-graph-database-neo4j-nebulagraph.md]] | 图数据库 | Neo4j Causal Cluster / NebulaGraph，Cypher / nGQL，知识图谱 |

### 分布式 SQL 与分析

| 文件 | 主题 | 核心技术 |
|------|------|---------|
| [[07-数据库中间件/08-新型数据库/03-newsql-cockroachdb-yugabytedb.md]] | NewSQL 数据库 | CockroachDB Operator / YugabyteDB，多区域部署，强一致性 |
| [[07-数据库中间件/08-新型数据库/04-olap-starrocks-doris-pinot.md]] | OLAP 分析引擎 | StarRocks / Apache Doris / Pinot，物化视图，实时导入 |

### 连接管理与 Schema 演进

| 文件 | 主题 | 核心技术 |
|------|------|---------|
| [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]] | 数据库连接池 | PgBouncer / ProxySQL，Transaction Pooling，读写分离 |
| [[07-数据库中间件/08-新型数据库/06-schema-migration-flyway-gh-ost-atlas.md]] | Schema 迁移 | Flyway / gh-ost / Atlas / Liquibase，零停机 DDL，expand-contract |

### 可观测性与数据保护

| 文件 | 主题 | 核心技术 |
|------|------|---------|
| [[07-数据库中间件/08-新型数据库/07-database-observability-pmm.md]] | 数据库可观测性 | PMM / pg_stat_statements / Performance Schema，告警设计 |
| [[07-数据库中间件/08-新型数据库/08-backup-tooling-pgbackrest-walg.md]] | 备份工具链 | pgBackRest / WAL-G / XtraBackup，PITR，备份验证 |

### 数据治理与分片

| 文件 | 主题 | 核心技术 |
|------|------|---------|
| [[07-数据库中间件/08-新型数据库/09-data-catalog-lineage-datahub.md]] | 数据目录与血缘 | DataHub / Marquez / OpenLineage，元数据管理 |
| [[07-数据库中间件/08-新型数据库/10-shardingsphere-vitess-sharding.md]] | 分库分表 | ShardingSphere / Vitess，分片策略，在线 Reshard |

## 阅读建议

- **AI 平台工程师**：优先阅读 01（向量数据库）和 02（图数据库），这是 RAG 和知识图谱的基础设施
- **数据库 DBA / SRE**：重点关注 05（连接池）、06（Schema 迁移）、07（可观测性）、08（备份）
- **数据平台团队**：关注 04（OLAP）、09（数据目录）和 [[07-数据库中间件/06-数据流/index.md|06-数据流]]
- **架构师**：03（NewSQL）和 10（分片）提供分布式数据架构的核心决策参考

## 关联知识

本目录与以下知识库板块紧密关联：

- [[07-数据库中间件/01-数据库/index.md|01-数据库]]：传统关系型数据库（PostgreSQL / MySQL）的 K8s 运维基础
- [[07-数据库中间件/05-Operator管理/index.md|05-Operator管理]]：数据库 Operator 的 CRD 管理和生命周期运维
- [[07-数据库中间件/06-数据流/index.md|06-数据流]]：Kafka / Flink 数据管线，与 OLAP 和数据目录集成
- [[09-可观测性/index.md|09-可观测性]]：Prometheus / Grafana 监控体系，数据库监控是其重要组成
- [[12-可靠性/01-备份恢复/index.md|01-备份恢复]]：灾难恢复策略，数据库备份是核心实践
- [[15-AI基础设施/index.md|15-AI基础设施]]：GPU 集群和模型服务，向量数据库是其数据层

## Related

- [[07-数据库中间件/01-数据库/index.md|01-数据库]]
- [[07-数据库中间件/05-Operator管理/index.md|05-Operator管理]]
- [[07-数据库中间件/06-数据流/index.md|06-数据流]]
- [[09-可观测性/index.md|09-可观测性]]
- [[12-可靠性/01-备份恢复/index.md|01-备份恢复]]
