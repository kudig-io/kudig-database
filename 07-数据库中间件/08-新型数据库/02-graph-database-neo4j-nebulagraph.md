---
title: "图数据库 K8s 部署（Neo4j/NebulaGraph）"
description: "覆盖 Neo4j 和 NebulaGraph 在 Kubernetes 上的集群部署、图查询优化与运维实践"
summary: "图数据库应用场景（知识图谱/推荐/欺诈检测），Neo4j Causal Cluster 部署，NebulaGraph 三组件分离架构，Cypher 与 nGQL 查询语言对比，存储内存规划，备份恢复与故障排查"
category: 数据库中间件
tags:
- database
- graph-database
- neo4j
- nebulagraph
- knowledge-graph
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 20min
intent_queries:
- "Neo4j 如何在 K8s 上部署集群"
- "NebulaGraph 架构和运维"
- "图数据库选型对比"
trigger_keywords:
- 图数据库
- Neo4j
- NebulaGraph
- Cypher
- nGQL
- 知识图谱
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

# 图数据库 K8s 部署（Neo4j/NebulaGraph）

## 概述

图数据库以节点（Node）和边（Relationship）为基本存储单元，天然适合表达实体间的复杂关联关系。在 AI 驱动的知识图谱、实时推荐引擎、金融欺诈检测、社交网络分析等场景中，图数据库相比关系型数据库在关联查询上有数量级的性能优势。

本文聚焦 Neo4j 和 NebulaGraph 两款主流图数据库在 Kubernetes 上的生产级部署与运维，涵盖集群架构、查询优化、备份恢复和故障排查。图数据库通常作为 [[15-AI基础设施/]] 中知识图谱层的核心组件，与向量数据库配合构建完整的 RAG 系统。

## 架构与核心概念

### 应用场景

| 场景 | 典型查询模式 | 推荐数据库 | 数据规模 |
|------|------------|-----------|---------|
| 知识图谱 | 多跳路径查询、实体推理 | Neo4j / NebulaGraph | 亿级节点 |
| 实时推荐 | 协同过滤、关联推荐 | NebulaGraph | 十亿级边 |
| 欺诈检测 | 环路检测、异常子图 | Neo4j | 千万级节点 |
| 网络拓扑 | 最短路径、连通性分析 | Neo4j / NebulaGraph | 百万级节点 |
| 权限管理 | 层级继承、传递权限 | Neo4j | 百万级节点 |

### Neo4j 架构

Neo4j Causal Cluster 由以下角色组成：

- **Core Server**：参与 Raft 共识，处理读写请求（通常 3/5/7 个）
- **Read Replica**：异步复制，只处理读请求，可水平扩展
- **Driver**：客户端驱动，自动路由读写请求

存储引擎：
- **Page Cache**：将图数据（节点、关系、属性）缓存在内存中
- **Transaction Log**：WAL 日志，保证 ACID
- **Store Files**：neostore.nodestore.db / neostore.relationshipstore.db 等

### NebulaGraph 架构

NebulaGraph 采用存储与计算分离的分布式架构：

- **Graph Service**：查询引擎层，解析 nGQL、生成执行计划（无状态，可水平扩展）
- **Meta Service**：元数据管理，存储 Schema、分区信息、用户权限（基于 Raft，通常 3 节点）
- **Storage Service**：数据存储层，基于 RocksDB，支持分片和副本（通常 3+ 节点）

### 查询语言对比

| 特性 | Cypher (Neo4j) | nGQL (NebulaGraph) |
|------|---------------|-------------------|
| 语法风格 | 声明式、ASCII Art 模式 | 类 SQL + 图遍历 |
| 模式匹配 | `MATCH (a)-[:KNOWS]->(b)` | `GO FROM "a" OVER knows YIELD b` |
| 路径查询 | `shortestPath()` | `FIND SHORTEST PATH` |
| 子图查询 | `CALL { ... }` | `GET SUBGRAPH` |
| 索引 | Schema Index / Full-text | Tag Index / Edge Index |
| 事务 | 完整 ACID | 部分支持（单分片） |
| 学习曲线 | 中等 | 中等 |

## 生产部署

### Neo4j Causal Cluster（Helm）

```yaml
# 🟡 中风险：部署 Neo4j Causal Cluster（3 Core + 2 Read Replica）
# neo4j-values.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: neo4j-cluster-values
  namespace: graph-db
data:
  values.yaml: |
    core:
      numberOfServers: 3
      persistentVolume:
        enabled: true
        size: 100Gi
        storageClassName: gp3-encrypted
      resources:
        requests:
          cpu: "4"
          memory: 16Gi
        limits:
          cpu: "8"
          memory: 32Gi
      neo4j:
        password: "${NEO4J_PASSWORD}"
        edition: enterprise
        acceptLicenseAgreement: "yes"
      config:
        dbms.memory.heap.max_size: "16G"
        dbms.memory.pagecache.size: "12G"
        dbms.tx_log.rotation.retention_policy: "2 days"
        causal_clustering.minimum_core_cluster_size_at_formation: 3
        causal_clustering.minimum_core_cluster_size_at_runtime: 3
    readReplica:
      numberOfServers: 2
      resources:
        requests:
          cpu: "2"
          memory: 8Gi
        limits:
          cpu: "4"
          memory: 16Gi
      config:
        dbms.memory.heap.max_size: "8G"
        dbms.memory.pagecache.size: "6G"
```

```bash
# 🟡 中风险：安装 Neo4j 集群
helm repo add neo4j https://neo4j-contrib.github.io/neo4j-helm/
helm repo update
kubectl create namespace graph-db
helm install neo4j-cluster neo4j/neo4j \
  -n graph-db \
  -f neo4j-values.yaml \
  --set core.neo4j.password="$(kubectl get secret neo4j-creds -n graph-db -o jsonpath='{.data.password}' | base64 -d)" \
  --timeout 15m \
  --wait
```

### NebulaGraph 部署（Nebula Operator）

```yaml
# 🟡 中风险：使用 Nebula Operator 部署 NebulaGraph 集群
apiVersion: apps.nebula-graph.io/v1alpha1
kind: NebulaCluster
metadata:
  name: nebula-cluster
  namespace: graph-db
spec:
  graphd:
    replicas: 2
    resources:
      requests:
        cpu: "2"
        memory: 4Gi
      limits:
        cpu: "4"
        memory: 8Gi
    config:
      max_allowed_statements: "1024"
      num_worker_threads: "8"
  metad:
    replicas: 3
    resources:
      requests:
        cpu: "1"
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
    dataDir:
    - path: /usr/local/nebula/data/meta
      storageClassName: gp3-encrypted
      storageSize: 20Gi
  storaged:
    replicas: 3
    resources:
      requests:
        cpu: "4"
        memory: 8Gi
      limits:
        cpu: "8"
        memory: 16Gi
    dataDir:
    - path: /usr/local/nebula/data/storage
      storageClassName: gp3-encrypted
      storageSize: 100Gi
    config:
      rocksdb_block_cache: "4GB"
      num_io_threads: "8"
  reference:
    name: statefulsets.apps
    version: v1
  imagePullPolicy: IfNotPresent
  schedulerName: default-scheduler
```

### 存储与内存规划

**Neo4j 内存配置公式：**
```
总内存 = pagecache + heap + OS reserve
pagecache ≈ 节点数 × 15B + 关系数 × 34B + 属性数 × 40B
heap ≈ 并发查询数 × 每查询内存（通常 2-8GB 足够）
OS reserve ≥ 2GB
```

**NebulaGraph 存储规划：**
```
Storage 磁盘 ≈ 原始数据 × 副本数 × 1.5（压缩+索引开销）
RocksDB Block Cache ≈ 热数据量（通常设为可用内存的 60%）
```

## 运维操作

### Neo4j 备份

```bash
# 🟡 中风险：执行 Neo4j 在线备份（Enterprise Edition）
kubectl exec -n graph-db neo4j-core-0 -- \
  neo4j-admin database backup neo4j \
  --to-path=/backups/ \
  --type=full \
  --verbose

# 🟢 低风险：查看备份文件
kubectl exec -n graph-db neo4j-core-0 -- ls -la /backups/
```

### NebulaGraph 快照

```bash
# 🟡 中风险：创建 NebulaGraph Storage 快照
kubectl exec -n graph-db nebula-cluster-graphd-0 -- \
  nebula-console -addr 127.0.0.1 -port 9669 -u root -p "${NEBULA_PASSWORD}" \
  -e "CREATE SNAPSHOT;"

# 🟢 低风险：查看快照状态
kubectl exec -n graph-db nebula-cluster-graphd-0 -- \
  nebula-console -addr 127.0.0.1 -port 9669 -u root -p "${NEBULA_PASSWORD}" \
  -e "SHOW SNAPSHOTS;"
```

### 集群健康检查

```bash
# 🟢 低风险：Neo4j 集群状态检查
kubectl exec -n graph-db neo4j-core-0 -- \
  cypher-shell -u neo4j -p "${NEO4J_PASSWORD}" \
  "CALL dbms.cluster.overview() YIELD memberId, role, addresses RETURN memberId, role, addresses"

# 🟢 低风险：NebulaGraph 集群状态
kubectl exec -n graph-db nebula-cluster-graphd-0 -- \
  nebula-console -addr 127.0.0.1 -port 9669 -u root -p "${NEBULA_PASSWORD}" \
  -e "SHOW HOSTS;"
```

## 故障排查

### Neo4j 集群脑裂

**现象**：多个 Core 节点同时认为自己是 Leader，写入冲突。

```bash
# 🟢 低风险：检查 Raft 集群状态
kubectl exec -n graph-db neo4j-core-0 -- \
  cypher-shell -u neo4j -p "${NEO4J_PASSWORD}" \
  "CALL dbms.cluster.raftState()"

# 🟢 低风险：查看集群日志中的 Raft 事件
kubectl logs -n graph-db neo4j-core-0 --tail=200 | grep -i "raft\|leader\|election"
```

**解决方案**：
1. 确认网络分区是否恢复
2. 检查 `causal_clustering.minimum_core_cluster_size_at_runtime` 配置
3. 必要时重启少数派节点让其重新加入集群

### NebulaGraph Storage 节点离线

```bash
# 🟢 低风险：检查 Storage 节点状态
kubectl exec -n graph-db nebula-cluster-graphd-0 -- \
  nebula-console -addr 127.0.0.1 -port 9669 -u root -p "${NEBULA_PASSWORD}" \
  -e "SHOW HOSTS STORAGE;"

# 🟡 中风险：手动平衡数据分区（确认节点恢复后）
kubectl exec -n graph-db nebula-cluster-graphd-0 -- \
  nebula-console -addr 127.0.0.1 -port 9669 -u root -p "${NEBULA_PASSWORD}" \
  -e "BALANCE DATA;"
```

### 查询性能问题

```bash
# 🟢 低风险：Neo4j 慢查询分析
kubectl exec -n graph-db neo4j-core-0 -- \
  cypher-shell -u neo4j -p "${NEO4J_PASSWORD}" \
  "CALL dbms.listQueries() YIELD queryId, query, elapsedTime WHERE elapsedTime > 5000 RETURN queryId, query, elapsedTime"

# 🟢 低风险：NebulaGraph 执行计划分析
kubectl exec -n graph-db nebula-cluster-graphd-0 -- \
  nebula-console -addr 127.0.0.1 -port 9669 -u root -p "${NEBULA_PASSWORD}" \
  -e "EXPLAIN GO FROM 'vertex_id' OVER edge_type YIELD edge_type._dst;"
```

## 最佳实践

1. **Neo4j Page Cache 调优**：将 `dbms.memory.pagecache.size` 设为总数据文件大小，避免磁盘 I/O 成为瓶颈
2. **NebulaGraph 分片策略**：根据数据量合理设置 Partition 数（建议每个 Storage 节点 5-10 个 Partition）
3. **索引策略**：对高频查询的属性建立索引，但避免过多索引影响写入性能
4. **读写分离**：Neo4j 使用 Read Replica 分担读负载；NebulaGraph 的 Graph Service 天然无状态可扩展
5. **备份策略**：参考 [[12-可靠性/01-备份恢复/]] 建立定期备份机制，Neo4j 使用 `neo4j-admin backup`，NebulaGraph 使用 Snapshot + BR 工具
6. **监控集成**：将 Neo4j Metrics / NebulaGraph Stats 接入 [[09-可观测性/]] 平台
7. **数据导入**：大批量数据导入使用 `neo4j-admin import`（离线）或 NebulaGraph Importer，避免逐条 INSERT
8. **Operator 管理**：使用 Nebula Operator 管理集群生命周期，参考 [[07-数据库中间件/05-Operator管理/]]

## Related

- [[07-数据库中间件/01-数据库/]]
- [[15-AI基础设施/]]
- [[12-可靠性/01-备份恢复/]]
- [[09-可观测性/]]
- [[07-数据库中间件/05-Operator管理/]]
