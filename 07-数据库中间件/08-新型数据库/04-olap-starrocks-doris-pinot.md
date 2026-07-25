---
title: "OLAP 分析引擎（StarRocks/Doris/Pinot）"
description: "覆盖 StarRocks、Apache Doris、Apache Pinot 在 Kubernetes 上的 OLAP 部署与查询优化"
summary: "OLAP vs OLTP 架构差异，StarRocks FE/BE 存算分离部署，Apache Doris 部署，Pinot Controller/Broker/Server 架构，实时/批量数据导入，物化视图与分区分桶优化，与 ClickHouse 对比，故障排查"
category: 数据库中间件
tags:
- database
- olap
- starrocks
- doris
- pinot
- analytics
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
- "StarRocks 如何在 K8s 上部署"
- "OLAP 引擎选型对比"
- "Apache Doris 运维实践"
trigger_keywords:
- OLAP
- StarRocks
- Doris
- Pinot
- ClickHouse
- 分析引擎
- 物化视图
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

# OLAP 分析引擎（StarRocks/Doris/Pinot）

## 概述

OLAP（Online Analytical Processing）引擎专为大规模数据分析查询设计，与 OLTP 数据库在存储格式、查询模式和扩展策略上有本质区别。StarRocks、Apache Doris 和 Apache Pinot 是当前主流的开源 OLAP 引擎，广泛应用于实时报表、用户行为分析、日志分析和 BI 看板等场景。

本文覆盖这三款引擎在 Kubernetes 上的生产部署、数据导入策略、查询优化和故障排查，帮助数据平台团队构建高性能分析基础设施。OLAP 引擎通常与 [[07-数据库中间件/06-数据流/]] 中的 Kafka/Flink 管线配合，实现实时数据入仓。

## 架构与核心概念

### OLAP vs OLTP 架构差异

| 维度 | OLTP（MySQL/PostgreSQL） | OLAP（StarRocks/Doris/Pinot） |
|------|------------------------|------------------------------|
| 查询模式 | 点查、短事务 | 聚合、扫描、多表 JOIN |
| 数据量 | GB-TB | TB-PB |
| 存储格式 | 行存（Row-based） | 列存（Columnar） |
| 索引策略 | B-Tree（点查优化） | 稀疏索引 + 位图 + Bloom Filter |
| 写入模式 | 高频小事务 | 批量导入 / 实时追加 |
| 一致性 | 强一致（ACID） | 最终一致 / 读时合并 |
| 扩展方式 | 垂直为主 | 水平扩展（MPP） |
| 典型延迟 | < 10ms | 100ms - 数秒 |

### StarRocks 架构

- **FE（Frontend）**：SQL 解析、查询规划、元数据管理（基于 BDB JE 共识）
  - Leader FE：处理写入和 DDL
  - Follower FE：参与选举，可读
  - Observer FE：只读扩展
- **BE（Backend）**：数据存储和查询执行（MPP 向量化引擎）
- **存算分离模式（3.0+）**：数据存储在 S3/HDFS，BE 变为无状态 CN（Compute Node）

### Apache Doris 架构

- **FE（Frontend）**：与 StarRocks 类似（StarRocks 最初 fork 自 Doris）
- **BE（Backend）**：存储 + 计算，支持多种数据模型（Duplicate/Aggregate/Unique）
- **Broker**：外部数据源读取代理

### Apache Pinot 架构

- **Controller**：集群管理、Schema/Table 配置、Segment 分配
- **Broker**：查询路由，合并多 Server 结果
- **Server**：
  - Realtime Server：消费 Kafka 实时数据
  - Offline Server：存储批量导入的 Segment
- **Minion**：后台任务（Segment 合并、数据清理）
- **依赖**：Apache ZooKeeper / Helix（集群协调）

### 与 ClickHouse 对比

| 特性 | StarRocks | Apache Doris | Apache Pinot | ClickHouse |
|------|-----------|-------------|-------------|-----------|
| 查询引擎 | 全面向量化 | 向量化 | 预计算为主 | 向量化 |
| 实时摄入 | 强（Routine Load） | 强（Routine Load） | 极强（原生 Kafka） | 中（需外部工具） |
| JOIN 能力 | 强（CBO 优化器） | 强 | 弱（有限 JOIN） | 中（大表 JOIN 弱） |
| 并发查询 | 高（千级 QPS） | 高 | 极高（万级 QPS） | 中（百级） |
| 运维复杂度 | 低 | 低 | 高（依赖 ZK） | 中 |
| 适用场景 | 统一分析 | 统一分析 | 实时指标/用户分析 | 日志/时序分析 |
| K8s 支持 | Operator | Operator | Helm | Operator |

## 生产部署

### StarRocks 部署（Operator）

```yaml
# 🟡 中风险：使用 StarRocks Operator 部署集群
apiVersion: starrocks.com/v1
kind: StarRocksCluster
metadata:
  name: sr-production
  namespace: olap
spec:
  starRocksFeSpec:
    replicas: 3
    resources:
      requests:
        cpu: "4"
        memory: 16Gi
      limits:
        cpu: "8"
        memory: 32Gi
    storageVolumes:
    - name: fe-meta
      storageClassName: gp3-encrypted
      storageSize: 50Gi
    - name: fe-log
      storageClassName: gp3-encrypted
      storageSize: 20Gi
    configMapInfo:
      configMapName: sr-fe-config
  starRocksBeSpec:
    replicas: 5
    resources:
      requests:
        cpu: "8"
        memory: 32Gi
      limits:
        cpu: "16"
        memory: 64Gi
    storageVolumes:
    - name: be-data
      storageClassName: gp3-encrypted
      storageSize: 500Gi
    configMapInfo:
      configMapName: sr-be-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: sr-be-config
  namespace: olap
data:
  be.conf: |
    storage_root_path = /opt/starrocks/be/storage
    mem_limit = 80%
    storage_page_cache_limit = 20%
    disable_storage_page_cache = false
    tc_use_memory_min = 10737418240
    chunk_reserved_bytes_limit = 2147483648
```

### Apache Doris 部署

```yaml
# 🟡 中风险：部署 Apache Doris 集群
apiVersion: doris.apache.org/v1
kind: DorisCluster
metadata:
  name: doris-production
  namespace: olap
spec:
  feSpec:
    replicas: 3
    resources:
      requests:
        cpu: "4"
        memory: 8Gi
      limits:
        cpu: "8"
        memory: 16Gi
    persistentVolumes:
    - mountPath: /opt/apache-doris/fe/doris-meta
      name: fe-meta
      persistentVolumeClaimSpec:
        storageClassName: gp3-encrypted
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 50Gi
  beSpec:
    replicas: 4
    resources:
      requests:
        cpu: "8"
        memory: 32Gi
      limits:
        cpu: "16"
        memory: 64Gi
    persistentVolumes:
    - mountPath: /opt/apache-doris/be/storage
      name: be-storage
      persistentVolumeClaimSpec:
        storageClassName: gp3-encrypted
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 500Gi
```

### Apache Pinot 部署（Helm）

```yaml
# 🟡 中风险：Pinot 集群 Helm values（含 Controller/Broker/Server/Minion）
# pinot-values.yaml
zookeeper:
  enabled: true
  replicaCount: 3
  persistence:
    enabled: true
    size: 10Gi

controller:
  replicaCount: 2
  resources:
    requests:
      cpu: "2"
      memory: 4Gi
    limits:
      cpu: "4"
      memory: 8Gi
  persistence:
    enabled: true
    size: 50Gi
    storageClass: gp3-encrypted

broker:
  replicaCount: 3
  resources:
    requests:
      cpu: "2"
      memory: 4Gi
    limits:
      cpu: "4"
      memory: 8Gi

server:
  replicaCount: 4
  resources:
    requests:
      cpu: "4"
      memory: 16Gi
    limits:
      cpu: "8"
      memory: 32Gi
  persistence:
    enabled: true
    size: 200Gi
    storageClass: gp3-encrypted

minion:
  replicaCount: 2
  resources:
    requests:
      cpu: "2"
      memory: 4Gi
```

## 运维操作

### 数据导入模式

```sql
-- 🟡 中风险：StarRocks Routine Load（从 Kafka 实时导入）
CREATE ROUTINE LOAD olap_db.kafka_ingest ON user_events
COLUMNS(event_time, user_id, event_type, properties)
PROPERTIES(
  "desired_concurrent_number" = "5",
  "max_batch_interval" = "20",
  "max_batch_rows" = "200000",
  "format" = "json",
  "strip_outer_array" = "true"
)
FROM KAFKA(
  "kafka_broker_list" = "kafka-0.kafka:9092,kafka-1.kafka:9092",
  "kafka_topic" = "user-events",
  "property.group.id" = "starrocks-consumer",
  "property.kafka_default_offsets" = "OFFSET_END"
);

-- 🟢 低风险：查看 Routine Load 状态
SHOW ROUTINE LOAD FOR olap_db.kafka_ingest;
```

### 查询优化

```sql
-- 🟡 中风险：创建物化视图加速聚合查询
CREATE MATERIALIZED VIEW mv_daily_active_users
AS
SELECT
  DATE(event_time) AS dt,
  event_type,
  COUNT(DISTINCT user_id) AS dau
FROM user_events
GROUP BY DATE(event_time), event_type;

-- 🟡 中风险：分区分桶策略（StarRocks）
CREATE TABLE user_events (
  event_time DATETIME,
  user_id BIGINT,
  event_type VARCHAR(50),
  properties JSON
)
PARTITION BY RANGE(event_time) (
  PARTITION p202607 VALUES LESS THAN ("2026-08-01"),
  PARTITION p202608 VALUES LESS THAN ("2026-09-01")
)
DISTRIBUTED BY HASH(user_id) BUCKETS 16
PROPERTIES (
  "replication_num" = "3",
  "storage_format" = "DEFAULT"
);
```

### 集群管理

```bash
# 🟢 低风险：查看 StarRocks BE 节点状态
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "SHOW BACKENDS\G"

# 🟢 低风险：查看当前查询
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "SHOW PROC '/current_queries';"

# 🟡 中风险：手动均衡 Tablet 分布
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "ADMIN SET FRONTEND CONFIG ('tablet_sched_max_scheduling_tablets' = '10000');"
```

## 故障排查

### BE 节点宕机

```bash
# 🟢 低风险：检查 BE 存活状态
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "SHOW BACKENDS;" | grep -i "alive"

# 🟢 低风险：查看 BE 日志
kubectl logs -n olap sr-production-be-0 --tail=200 | grep -i "error\|fatal\|oom"

# 🟡 中风险：BE 节点恢复后检查 Tablet 修复进度
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "SHOW PROC '/statistic';"
```

### 查询超时

排查路径：
1. 检查查询是否命中物化视图（`EXPLAIN` 查看执行计划）
2. 确认分区分桶是否合理（避免全表扫描）
3. 检查 BE 内存使用（是否触发 Spill to Disk）
4. 查看并发查询数是否超出 `qe_max_connection` 限制

```sql
-- 🟢 低风险：分析查询执行计划
EXPLAIN VERBOSE SELECT COUNT(DISTINCT user_id) FROM user_events WHERE event_time >= '2026-07-01';

-- 🟢 低风险：查看查询 Profile（性能分析）
SET enable_profile = true;
SELECT COUNT(DISTINCT user_id) FROM user_events WHERE event_time >= '2026-07-01';
SHOW PROFILELIST;
```

### 数据导入失败

```bash
# 🟢 低风险：查看 Routine Load 错误信息
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "SHOW ROUTINE LOAD\G" | grep -A5 "ErrorMsg"

# 🟢 低风险：查看 Load 任务状态
mysql -h sr-production-fe.olap.svc -P 9030 -u root -e "SHOW LOAD ORDER BY CreateTime DESC LIMIT 10\G"
```

## 最佳实践

1. **分区分桶设计**：按时间分区（便于 TTL 清理），按高基数列分桶（避免数据倾斜），桶数建议为 BE 节点数的 2-4 倍
2. **物化视图策略**：对高频聚合查询创建物化视图，定期刷新，参考 [[07-数据库中间件/06-数据流/]] 中的增量计算模式
3. **内存管理**：BE 的 `mem_limit` 设为容器内存的 80%，预留 20% 给 OS 和 Page Cache
4. **数据导入**：实时场景用 Routine Load / Kafka Connect；批量场景用 Broker Load / INSERT INTO SELECT
5. **副本策略**：生产环境 `replication_num = 3`，确保 BE 节点 >= 3
6. **监控告警**：关注 Compaction 积压、Tablet 不健康数、查询 P99 延迟，接入 [[09-可观测性/]] 平台
7. **备份恢复**：使用 `BACKUP/RESTORE` 命令备份到 S3/HDFS，参考 [[12-可靠性/01-备份恢复/]]
8. **资源隔离**：OLAP 集群使用独立 Node Pool，避免与 OLTP 工作负载争抢资源，参考 [[07-数据库中间件/01-数据库/]] 中的资源管理

## Related

- [[07-数据库中间件/06-数据流/]]
- [[07-数据库中间件/01-数据库/]]
- [[09-可观测性/]]
- [[12-可靠性/01-备份恢复/]]
- [[07-数据库中间件/05-Operator管理/]]
