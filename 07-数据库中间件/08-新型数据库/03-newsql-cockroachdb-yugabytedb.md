---
title: "NewSQL 数据库（CockroachDB/YugabyteDB）"
description: "覆盖 CockroachDB 和 YugabyteDB 在 Kubernetes 上的分布式 SQL 部署、多区域架构与运维"
summary: "NewSQL 分布式 SQL 概念，CockroachDB Operator 与 CRDB Cluster CR 部署，YugabyteDB Master/TServer 架构，与 TiDB 对比，多区域数据本地性，备份恢复，节点故障/时钟偏移/事务冲突排查"
category: 数据库中间件
tags:
- database
- newsql
- cockroachdb
- yugabytedb
- distributed-sql
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
- "CockroachDB 如何在 K8s 上部署"
- "YugabyteDB 与 TiDB 对比"
- "NewSQL 分布式数据库运维"
trigger_keywords:
- NewSQL
- CockroachDB
- YugabyteDB
- 分布式SQL
- 强一致性
- 水平扩展
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

# NewSQL 数据库（CockroachDB/YugabyteDB）

## 概述

NewSQL 数据库结合了传统关系型数据库的 ACID 事务保证和 NoSQL 的水平扩展能力，提供分布式 SQL 接口。CockroachDB 和 YugabyteDB 是两款代表性的 NewSQL 数据库，均兼容 PostgreSQL 协议，支持在 Kubernetes 上原生部署。

与 [[07-数据库中间件/01-数据库/]] 中的传统关系型数据库相比，NewSQL 解决了单点瓶颈和手动分片的复杂性；与 TiDB 相比，CockroachDB/YugabyteDB 更侧重 PostgreSQL 生态兼容性和多区域部署能力。

## 架构与核心概念

### NewSQL 核心特征

- **分布式 SQL**：数据自动分片（Range/Tablet），对应用透明
- **强一致性**：支持 Serializable 隔离级别（CockroachDB）/ Snapshot Isolation（YugabyteDB）
- **水平扩展**：增加节点即可线性扩展吞吐和存储
- **高可用**：基于 Raft/Paxos 共识，自动故障转移
- **SQL 兼容**：兼容 PostgreSQL 协议，现有应用可低成本迁移

### CockroachDB 架构

- **SQL Layer**：解析、优化、执行 SQL
- **KV Layer**：分布式事务 KV 存储
- **Storage Layer**：基于 Pebble（RocksDB 替代）的 LSM-Tree 存储
- **Replication**：每个 Range（默认 512MB）通过 Raft 复制 3/5 副本
- **Gossip**：节点发现和元数据传播

### YugabyteDB 架构

- **YB-Master**：元数据管理（Tablet 分布、Schema、负载均衡），Raft 3 节点
- **YB-TServer**：数据存储和查询执行，基于 DocDB（RocksDB 增强）
- **Tablet**：数据分片单元，每个 Tablet 通过 Raft 复制 3 副本
- **API 层**：同时支持 YSQL（PostgreSQL 兼容）和 YCQL（Cassandra 兼容）

### 与 TiDB 对比

| 特性 | CockroachDB | YugabyteDB | TiDB |
|------|------------|-----------|------|
| SQL 兼容 | PostgreSQL | PostgreSQL + Cassandra | MySQL |
| 一致性模型 | Serializable | Snapshot (SI) | Snapshot (SI) |
| 存储引擎 | Pebble (LSM) | DocDB (RocksDB) | TiKV (RocksDB) |
| 共识协议 | Raft | Raft | Raft (Multi-Raft) |
| K8s Operator | 官方 CRDB Operator | 官方 YugabyteDB Operator | TiDB Operator |
| 多区域部署 | 原生支持（Locality） | 原生支持（Geo-Partition） | 支持（Placement Rules） |
| 事务模型 | 2PC + Parallel Commits | 2PC | Percolator |
| 时钟要求 | NTP（HLC） | NTP（HLC） | TSO（中心化） |
| 适用场景 | 全球分布式 OLTP | 多模型（SQL+NoSQL） | MySQL 替代、HTAP |

## 生产部署

### CockroachDB（Operator 部署）

```yaml
# 🟡 中风险：使用 CockroachDB Operator 部署集群
apiVersion: crdb.cockroachlabs.com/v1alpha1
kind: CrdbCluster
metadata:
  name: crdb-production
  namespace: newsql
spec:
  dataStore:
    pvc:
      spec:
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 200Gi
        storageClassName: gp3-encrypted
  resources:
    requests:
      cpu: "4"
      memory: 16Gi
    limits:
      cpu: "8"
      memory: 32Gi
  nodes: 5
  cockroachDBVersion: v24.1.5
  additionalLabels:
    app: cockroachdb
    environment: production
  podEnvVars:
  - name: COCKROACH_MAX_OFFSET
    value: "500ms"
  tlsEnabled: true
  clientTLSSecret: crdb-client-tls
  nodeTLSSecret: crdb-node-tls
```

```bash
# 🟡 中风险：安装 CockroachDB Operator 并创建集群
kubectl apply -f https://raw.githubusercontent.com/cockroachdb/cockroach-operator/master/install/crds.yaml
kubectl apply -f https://raw.githubusercontent.com/cockroachdb/cockroach-operator/master/install/operator.yaml
kubectl create namespace newsql
kubectl apply -f crdb-cluster.yaml -n newsql
kubectl wait --for=condition=Initialized crdbcluster/crdb-production -n newsql --timeout=600s
```

### YugabyteDB 部署

```yaml
# 🟡 中风险：部署 YugabyteDB 集群（3 Master + 5 TServer）
apiVersion: yugabytedb.io/v1alpha1
kind: YBCluster
metadata:
  name: ybdb-production
  namespace: newsql
spec:
  master:
    replicas: 3
    resources:
      requests:
        cpu: "2"
        memory: 8Gi
      limits:
        cpu: "4"
        memory: 16Gi
    storage:
      storageClassName: gp3-encrypted
      size: 50Gi
    gflags:
      master:
        master_failover_cold_start_ms: "30000"
  tserver:
    replicas: 5
    resources:
      requests:
        cpu: "4"
        memory: 16Gi
      limits:
        cpu: "8"
        memory: 32Gi
    storage:
      storageClassName: gp3-encrypted
      size: 200Gi
    gflags:
      tserver:
        rocksdb_block_cache_size_mb: "4096"
        yb_num_shards_per_tserver: "8"
        memstore_size_mb: "2048"
  ysql:
    enabled: true
    port: 5433
  ycql:
    enabled: true
    port: 9042
```

### 多区域部署与数据本地性

```sql
-- 🟡 中风险：CockroachDB 多区域配置（设置 Locality 和 Zone Config）
-- 启动节点时指定 locality：
-- --locality=region=us-east-1,zone=us-east-1a

-- 设置多区域生存策略
ALTER DATABASE myapp PRIMARY REGION "us-east-1";
ALTER DATABASE myapp ADD REGION "eu-west-1";
ALTER DATABASE myapp ADD REGION "ap-southeast-1";
ALTER DATABASE myapp SURVIVE REGION FAILURE;

-- 表级别数据本地性
ALTER TABLE users SET LOCALITY REGIONAL BY ROW AS region;
ALTER TABLE config SET LOCALITY GLOBAL;
```

## 运维操作

### 集群扩缩容

```bash
# 🟡 中风险：CockroachDB 扩容到 7 节点
kubectl patch crdbcluster crdb-production -n newsql \
  --type merge -p '{"spec":{"nodes":7}}'

# 🟢 低风险：查看 CockroachDB 节点状态
kubectl exec -n newsql crdb-production-0 -- \
  cockroach node status --certs-dir=/cockroach/cockroach-certs

# 🟢 低风险：查看 Range 分布
kubectl exec -n newsql crdb-production-0 -- \
  cockroach node status --certs-dir=/cockroach/cockroach-certs --format=csv
```

### 备份恢复

```sql
-- 🔴 高风险：CockroachDB 全量备份到 S3
BACKUP INTO 's3://backup-bucket/crdb/full?AUTH=implicit'
  AS OF SYSTEM TIME '-10s'
  WITH revision_history;

-- 🔴 高风险：增量备份
BACKUP INTO LATEST IN 's3://backup-bucket/crdb/full?AUTH=implicit'
  AS OF SYSTEM TIME '-10s';

-- 🔴 高风险：恢复数据库（会覆盖现有数据）
RESTORE DATABASE myapp FROM LATEST IN 's3://backup-bucket/crdb/full?AUTH=implicit'
  WITH new_db_name = 'myapp_restored';
```

### 集群健康监控

```bash
# 🟢 低风险：CockroachDB 集群健康检查
kubectl exec -n newsql crdb-production-0 -- \
  cockroach sql --certs-dir=/cockroach/cockroach-certs \
  -e "SELECT * FROM crdb_internal.cluster_health;"

# 🟢 低风险：查看 Range 副本健康
kubectl exec -n newsql crdb-production-0 -- \
  cockroach sql --certs-dir=/cockroach/cockroach-certs \
  -e "SELECT * FROM crdb_internal.ranges WHERE array_length(replicas, 1) < 3;"
```

## 故障排查

### 节点故障与恢复

```bash
# 🟢 低风险：检查 CockroachDB 节点存活状态
kubectl exec -n newsql crdb-production-0 -- \
  cockroach node status --certs-dir=/cockroach/cockroach-certs --decommission

# 🟡 中风险：安全下线节点（先 decommission 再删除 Pod）
kubectl exec -n newsql crdb-production-0 -- \
  cockroach node decommission 4 --certs-dir=/cockroach/cockroach-certs --wait=none

# 等待 Range 迁移完成后再缩容
kubectl patch crdbcluster crdb-production -n newsql \
  --type merge -p '{"spec":{"nodes":4}}'
```

### 时钟偏移

CockroachDB 使用 Hybrid Logical Clock (HLC)，要求节点间时钟偏差在 `max_offset`（默认 500ms）以内：

```bash
# 🟢 低风险：检查节点时钟偏移
kubectl exec -n newsql crdb-production-0 -- \
  cockroach sql --certs-dir=/cockroach/cockroach-certs \
  -e "SELECT node_id, activity, round((now() - start_time)::numeric, 2) as uptime FROM crdb_internal.gossip_nodes;"

# 🟢 低风险：检查 NTP 同步状态
kubectl exec -n newsql crdb-production-0 -- chronyc tracking
```

**时钟偏移过大的后果**：节点被踢出集群（clock synchronization error），需要修复 NTP 后重启节点。

### 事务冲突与重试

```sql
-- 🟢 低风险：查看事务冲突统计
SELECT * FROM crdb_internal.node_txn_stats
WHERE txn_restarts > 0
ORDER BY txn_restarts DESC
LIMIT 20;

-- 🟢 低风险：查看当前活跃事务
SELECT * FROM crdb_internal.cluster_transactions
WHERE status = 'PENDING'
ORDER BY txn_start ASC;
```

**常见事务冲突原因**：
1. **Write-Write 冲突**：多个事务修改同一行，使用 `SELECT ... FOR UPDATE` 提前加锁
2. **Read-Write 冲突（Uncertainty）**：读取到未来时间戳的写入，应用层需实现自动重试
3. **死锁**：交叉更新多行，统一加锁顺序

## 最佳实践

1. **节点数量**：CockroachDB 最少 3 节点（容忍 1 节点故障），生产建议 5+ 节点
2. **时钟同步**：必须部署 chrony/NTP，确保偏差 < 250ms，参考 [[09-可观测性/]] 设置时钟偏移告警
3. **存储配置**：使用 SSD，`--store` 指定独立磁盘，避免与系统盘共享 I/O
4. **连接管理**：使用 PgBouncer 或内置连接池，参考 [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]
5. **备份策略**：每日全量 + 每小时增量，存储到 S3/GCS，参考 [[12-可靠性/01-备份恢复/]]
6. **Locality 配置**：多 AZ 部署时指定 `--locality` 确保副本分布合理
7. **Schema 变更**：使用在线 DDL（`ALTER TABLE ... ADD COLUMN`），大表变更参考 [[07-数据库中间件/08-新型数据库/06-schema-migration-flyway-gh-ost-atlas.md]]
8. **监控告警**：关注 Range 副本不足、证书过期、磁盘使用率 > 80% 等关键指标

## Related

- [[07-数据库中间件/01-数据库/]]
- [[12-可靠性/01-备份恢复/]]
- [[09-可观测性/]]
- [[07-数据库中间件/05-Operator管理/]]
- [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]
