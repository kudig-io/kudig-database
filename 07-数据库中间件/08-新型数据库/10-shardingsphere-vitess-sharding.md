---
title: "分库分表（ShardingSphere/Vitess）"
description: "覆盖 ShardingSphere 和 Vitess 在 Kubernetes 上的分片部署、分片键设计与在线迁移"
summary: "分片策略（Hash/Range/时间/一致性哈希），ShardingSphere Proxy vs JDBC 部署，Vitess VTGate/VTTablet/etcd 架构，分片键选择，跨分片查询优化，在线分片迁移，热点分片与数据倾斜排查"
category: 数据库中间件
tags:
- database
- sharding
- shardingsphere
- vitess
- mysql
- distributed-database
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
- "ShardingSphere 如何在 K8s 上部署"
- "Vitess 分片架构和运维"
- "分库分表如何选择分片键"
trigger_keywords:
- 分库分表
- ShardingSphere
- Vitess
- 分片
- sharding
- 数据倾斜
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

# 分库分表（ShardingSphere/Vitess）

## 概述

当单表数据量超过千万级或单库 QPS 超过万级时，垂直扩展（加硬件）的边际效益急剧下降，水平分片（Sharding）成为必然选择。分库分表将数据按规则分散到多个数据库实例和表中，突破单机存储和性能瓶颈。

ShardingSphere 和 Vitess 是两款主流的分片中间件：ShardingSphere 更贴近 Java 生态，提供 JDBC 和 Proxy 两种接入模式；Vitess 由 YouTube 开源，是 MySQL 分片的工业级方案，原生支持 Kubernetes。本文覆盖两者在 K8s 上的部署、分片策略设计和运维实践。分片方案通常与 [[07-数据库中间件/01-数据库/]] 中的 MySQL/PostgreSQL 集群配合使用。

## 架构与核心概念

### 分片策略对比

| 策略 | 原理 | 优点 | 缺点 | 适用场景 |
|------|------|------|------|---------|
| **Hash 分片** | hash(shard_key) % N | 数据均匀分布 | 扩容需 rehash | 用户表、订单表 |
| **Range 分片** | 按值范围划分 | 扩容方便（加新范围） | 易产生热点 | 时间序列数据 |
| **时间分片** | 按日/月/年划分 | 天然支持 TTL 清理 | 近期数据热点 | 日志表、事件表 |
| **一致性哈希** | 虚拟节点环 | 扩容只迁移部分数据 | 实现复杂 | 缓存层分片 |
| **目录分片** | 查找表映射 | 灵活 | 查找表成为瓶颈 | 多租户 SaaS |

### ShardingSphere 架构

ShardingSphere 提供两种部署模式：

**ShardingSphere-JDBC（嵌入式）：**
- 以 JAR 包形式嵌入 Java 应用
- 无额外网络跳转，性能最优
- 仅支持 Java 生态
- 无中心化组件，每个应用实例独立路由

**ShardingSphere-Proxy（独立代理）：**
- 独立部署的数据库代理进程
- 兼容 MySQL/PostgreSQL 协议，任何语言可接入
- 对应用透明，无需修改代码
- 需要额外运维代理集群

### Vitess 架构

Vitess 是 MySQL 的水平扩展方案，核心组件：

- **VTGate**：无状态查询路由层，解析 SQL 并分发到正确的 Shard
- **VTTablet**：每个 MySQL 实例旁的 Sidecar 代理，管理连接池、查询重写、复制监控
- **VTAdmin**：管理 API 和 Web UI
- **Topology Service**：存储集群元数据（etcd / ZooKeeper / Consul）
- **VTBackup**：备份管理
- **VTOrc**：故障检测和自动主从切换

### ShardingSphere vs Vitess 对比

| 特性 | ShardingSphere | Vitess |
|------|---------------|--------|
| 数据库支持 | MySQL + PostgreSQL | 仅 MySQL |
| 部署模式 | JDBC / Proxy | Proxy（VTGate） |
| K8s 原生支持 | 需自行部署 | 官方 Operator |
| 在线分片迁移 | 支持（ElasticJob） | 原生支持（VReplication） |
| 跨分片事务 | XA / BASE | 2PC（有限） |
| 查询兼容性 | 高（大部分 SQL） | 中（部分 SQL 不支持） |
| 社区生态 | Apache 顶级项目 | CNCF 毕业项目 |
| 适用规模 | 中大型 | 超大型（YouTube 验证） |
| 运维复杂度 | 中 | 高 |

## 生产部署

### ShardingSphere-Proxy 部署

```yaml
# 🟡 中风险：ShardingSphere-Proxy 集群部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: shardingsphere-proxy
  namespace: database
spec:
  replicas: 3
  selector:
    matchLabels:
      app: shardingsphere-proxy
  template:
    metadata:
      labels:
        app: shardingsphere-proxy
    spec:
      containers:
      - name: proxy
        image: apache/shardingsphere-proxy:5.5.0
        ports:
        - containerPort: 3307
          name: mysql
        resources:
          requests:
            cpu: "2"
            memory: 4Gi
          limits:
            cpu: "4"
            memory: 8Gi
        volumeMounts:
        - name: config
          mountPath: /opt/shardingsphere-proxy/conf
        livenessProbe:
          tcpSocket:
            port: 3307
          initialDelaySeconds: 30
          periodSeconds: 15
        readinessProbe:
          tcpSocket:
            port: 3307
          initialDelaySeconds: 15
          periodSeconds: 10
      volumes:
      - name: config
        configMap:
          name: shardingsphere-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: shardingsphere-config
  namespace: database
data:
  server.yaml: |
    mode:
      type: Cluster
      repository:
        type: ZooKeeper
        props:
          namespace: shardingsphere
          server-lists: zk-0.zk:2181,zk-1.zk:2181,zk-2.zk:2181
          retryIntervalMilliseconds: 500
          timeToLiveSeconds: 60
          maxRetries: 3
          operationTimeoutMilliseconds: 500
    proxy:
      max-connections-size-per-query: 1
      kernel-executor-size: 16
      proxy-frontend-flush-threshold: 128
      proxy-default-port: 3307
  config-sharding.yaml: |
    databaseName: myapp_sharding
    dataSources:
      ds_0:
        dataSourceClassName: com.zaxxer.hikari.HikariDataSource
        jdbcUrl: jdbc:mysql://mysql-shard-0.database.svc:3306/myapp_0
        username: ${DB_USER}
        password: ${DB_PASSWORD}
        maximumPoolSize: 50
      ds_1:
        dataSourceClassName: com.zaxxer.hikari.HikariDataSource
        jdbcUrl: jdbc:mysql://mysql-shard-1.database.svc:3306/myapp_1
        username: ${DB_USER}
        password: ${DB_PASSWORD}
        maximumPoolSize: 50
      ds_2:
        dataSourceClassName: com.zaxxer.hikari.HikariDataSource
        jdbcUrl: jdbc:mysql://mysql-shard-2.database.svc:3306/myapp_2
        username: ${DB_USER}
        password: ${DB_PASSWORD}
        maximumPoolSize: 50
      ds_3:
        dataSourceClassName: com.zaxxer.hikari.HikariDataSource
        jdbcUrl: jdbc:mysql://mysql-shard-3.database.svc:3306/myapp_3
        username: ${DB_USER}
        password: ${DB_PASSWORD}
        maximumPoolSize: 50
    rules:
    - !SHARDING
      tables:
        orders:
          actualDataNodes: ds_${0..3}.orders_${0..15}
          tableStrategy:
            standard:
              shardingColumn: order_id
              shardingAlgorithmName: orders_table_hash
          databaseStrategy:
            standard:
              shardingColumn: user_id
              shardingAlgorithmName: orders_db_hash
          keyGenerateStrategy:
            column: order_id
            keyGeneratorName: snowflake
      shardingAlgorithms:
        orders_db_hash:
          type: HASH_MOD
          props:
            sharding-count: 4
        orders_table_hash:
          type: HASH_MOD
          props:
            sharding-count: 16
      keyGenerators:
        snowflake:
          type: SNOWFLAKE
```

### Vitess 部署（Operator）

```yaml
# 🟡 中风险：Vitess 集群部署（Vitess Operator）
apiVersion: planetscale.com/v2
kind: VitessCluster
metadata:
  name: vitess-prod
  namespace: database
spec:
  images:
    vtgate: vitess/lite:v19.0.0
    vttablet: vitess/lite:v19.0.0
    vtbackup: vitess/lite:v19.0.0
    mysqld:
      mysql80Compatible: vitess/lite:v19.0.0
    vtctld: vitess/lite:v19.0.0
  cells:
  - name: zone1
    gateway:
      replicas: 3
      resources:
        requests:
          cpu: "2"
          memory: 4Gi
        limits:
          cpu: "4"
          memory: 8Gi
  keyspaces:
  - name: myapp
    turndownPolicy: RequireIdle
    partitionings:
    - equal:
        parts: 8
        shardTemplate:
          databaseInitScriptSecret:
            name: vitess-schema-init
          tabletPools:
          - cell: zone1
            type: replica
            replicas: 2
            vttablet:
              resources:
                requests:
                  cpu: "4"
                  memory: 8Gi
                limits:
                  cpu: "8"
                  memory: 16Gi
              extraFlags:
                queryserver-config-pool-size: "300"
                queryserver-config-stream-pool-size: "200"
            mysqld:
              resources:
                requests:
                  cpu: "4"
                  memory: 8Gi
              configOverrides: |
                innodb_buffer_pool_size = 6G
                innodb_log_file_size = 1G
          - cell: zone1
            type: primary
            replicas: 1
            vttablet:
              resources:
                requests:
                  cpu: "4"
                  memory: 8Gi
  etcd:
    createEtcdClusters:
      zone1:
        replicas: 3
        resources:
          requests:
            cpu: "500m"
            memory: 1Gi
        dataVolumeClaimTemplate:
          accessModes: ["ReadWriteOnce"]
          storageClassName: gp3-encrypted
          resources:
            requests:
              storage: 10Gi
```

## 运维操作

### 分片键选择原则

```sql
-- 分片键选择检查清单：
-- 1. 高基数（避免数据倾斜）
-- 2. 查询频率高（大部分查询能路由到单分片）
-- 3. 不可变（分片键值不会变化）
-- 4. 均匀分布（避免时间戳作为 Hash 分片键）

-- 🟢 低风险：分析查询模式确定分片键
-- 统计 WHERE 条件中各列出现频率
SELECT
  column_name,
  count(*) as query_frequency
FROM information_schema.statistics
WHERE table_name = 'orders'
GROUP BY column_name
ORDER BY query_frequency DESC;

-- 🟢 低风险：检查数据分布均匀性（ShardingSphere）
-- 通过 Proxy 管理端口
mysql -h shardingsphere-proxy.database.svc -P 3307 -u root -p \
  -e "SELECT ds_0.count, ds_1.count, ds_2.count, ds_3.count FROM (SELECT count(*) as count FROM orders WHERE user_id % 4 = 0) ds_0, (SELECT count(*) as count FROM orders WHERE user_id % 4 = 1) ds_1, (SELECT count(*) as count FROM orders WHERE user_id % 4 = 2) ds_2, (SELECT count(*) as count FROM orders WHERE user_id % 4 = 3) ds_3;"
```

### 跨分片查询优化

```sql
-- 🟢 低风险：ShardingSphere 跨分片查询分析
-- 广播表（小表复制到所有分片，避免跨分片 JOIN）
-- 在 config-sharding.yaml 中配置：
-- broadcastTables:
--   - config
--   - region_code

-- 绑定表（相同分片策略的表，JOIN 时不跨分片）
-- bindingTables:
--   - orders,order_items  （两者都按 user_id 分片）

-- Vitess 跨分片查询限制：
-- 不支持跨分片的子查询、UNION（需应用层合并）
-- 使用 vschema 定义路由规则
```

### Vitess 在线分片迁移（VReplication）

```bash
# 🔴 高风险：Vitess 在线 Reshard（从 4 分片扩展到 8 分片）
# 1. 创建新的 Shard
vtctlclient --server vtctld.database.svc:15999 CreateShard -force myapp/-40
vtctlclient --server vtctld.database.svc:15999 CreateShard -force myapp/40-80
vtctlclient --server vtctld.database.svc:15999 CreateShard -force myapp/80-c0
vtctlclient --server vtctld.database.svc:15999 CreateShard -force myapp/c0-

# 2. 启动 VReplication（数据复制）
vtctlclient --server vtctld.database.svc:15999 Reshard --create myapp.orders_reshard \
  --source_shards "-80,80-" \
  --target_shards "-40,40-80,80-c0,c0-"

# 3. 监控复制进度
vtctlclient --server vtctld.database.svc:15999 ShowWorkflow --workflow orders_reshard --keyspace myapp

# 4. 切换流量（SwitchTraffic）
vtctlclient --server vtctld.database.svc:15999 Reshard --switch_traffic myapp.orders_reshard

# 5. 清理旧分片
vtctlclient --server vtctld.database.svc:15999 Reshard --complete myapp.orders_reshard
```

## 故障排查

### 热点分片

**现象**：某个分片的 CPU/IOPS 远高于其他分片。

```bash
# 🟢 低风险：ShardingSphere 查看各分片查询分布
mysql -h shardingsphere-proxy.database.svc -P 3307 -u root -p \
  -e "SHOW SHARDING TABLE STATUS;"

# 🟢 低风险：Vitess 查看各 Shard 的 QPS
vtctlclient --server vtctld.database.svc:15999 GetShardReplication zone1 myapp/-80

# 🟢 低风险：检查各 MySQL 分片负载
for i in 0 1 2 3; do
  echo "=== Shard $i ==="
  kubectl exec -n database mysql-shard-$i-0 -- \
    mysql -u monitor -p"${MONITOR_PWD}" -e "SHOW GLOBAL STATUS LIKE 'Queries'; SHOW GLOBAL STATUS LIKE 'Threads_running';"
done
```

**解决方案**：
1. 分析热点分片的查询模式（是否分片键选择不当）
2. 考虑增加二级分片（复合分片键）
3. 对热点数据增加缓存层（参考 [[07-数据库中间件/02-缓存/]]）
4. 长期方案：重新设计分片策略并执行在线迁移

### 数据倾斜

```sql
-- 🟢 低风险：检查各分片数据量分布
-- ShardingSphere
SELECT 'ds_0' as shard, COUNT(*) as cnt FROM orders_0 UNION ALL
SELECT 'ds_1', COUNT(*) FROM orders_1 UNION ALL
SELECT 'ds_2', COUNT(*) FROM orders_2 UNION ALL
SELECT 'ds_3', COUNT(*) FROM orders_3;

-- 🟢 低风险：检查分片键值分布
SELECT user_id % 4 as shard_id, COUNT(*) as cnt
FROM orders
GROUP BY user_id % 4
ORDER BY shard_id;
```

### 跨分片事务失败

**现象**：分布式事务超时或部分提交。

```bash
# 🟢 低风险：ShardingSphere 查看 XA 事务状态
mysql -h shardingsphere-proxy.database.svc -P 3307 -u root -p \
  -e "XA RECOVER;"

# 🟢 低风险：各分片检查悬挂事务
for i in 0 1 2 3; do
  echo "=== Shard $i ==="
  kubectl exec -n database mysql-shard-$i-0 -- \
    mysql -u root -p"${ROOT_PWD}" -e "XA RECOVER CONVERT XID;"
done
```

**解决方案**：
1. 尽量设计避免跨分片事务（通过分片键设计）
2. 使用最终一致性（Saga 模式）替代强一致 XA
3. 设置合理的事务超时时间
4. 监控悬挂 XA 事务并定期清理

## 最佳实践

1. **分片键设计**：选择查询频率最高、基数大、不可变的列（如 user_id），避免时间戳
2. **分片数量**：初始分片数建议为预期峰值的 2-4 倍（如预期 4 个分片够用，初始设 8-16 个），避免频繁 Reshard
3. **广播表**：将小型配置表（< 1 万行）设为广播表，避免跨分片 JOIN
4. **绑定表**：关联查询的表使用相同分片策略，确保 JOIN 在单分片内完成
5. **全局 ID**：使用 Snowflake / UUID 生成全局唯一 ID，避免依赖单库自增
6. **监控**：关注各分片的 QPS/连接数/磁盘使用均衡性，接入 [[09-可观测性/]] 平台
7. **备份**：每个分片独立备份，参考 [[12-可靠性/01-备份恢复/]] 确保一致性快照
8. **连接池**：Proxy 模式下配合连接池使用，参考 [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]
9. **Operator 管理**：Vitess 使用官方 Operator，参考 [[07-数据库中间件/05-Operator管理/]] 中的 CRD 管理
10. **渐进式迁移**：从单库到分片使用双写 + 校验 + 切流的渐进策略，避免大爆炸迁移

## Related

- [[07-数据库中间件/01-数据库/]]
- [[07-数据库中间件/05-Operator管理/]]
- [[09-可观测性/]]
- [[12-可靠性/01-备份恢复/]]
- [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]
