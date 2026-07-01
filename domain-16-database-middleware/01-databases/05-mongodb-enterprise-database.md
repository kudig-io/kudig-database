---
title: MongoDB 企业级数据库运维深度实践
description: '# MongoDB 企业级数据库运维深度实践'
category: enterprise-database-middleware
tags:
- k8s
- database
- middleware
- mysql
- redis
- prometheus
- pdb
- statefulset
- rbac
- operator
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- DBA
- SRE
- 后端开发
estimated_read_time: 5min
intent_queries:
- MongoDB 企业级数据库运维深度实践 是什么
- 如何 MongoDB 企业级数据库运维深度实践
- Kubernetes 28 enterprise database middleware 最佳实践
trigger_keywords:
- MongoDB
- 企业级数据库运维深度实践
- enterprise
- database
- middleware
prerequisites:
- kubectl-basics
- prometheus-basics
- kafka-basics
- redis-basics
- mysql-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/sql.md
  label: '速查卡: sql'
created: "2026-05-23"
---

# MongoDB 企业级数据库运维深度实践

> **适用版本**: MongoDB 7.0 ~ 8.0  
> **最后更新**: 2026-04-26  
> **难度**: 中级 → 高级

---

<!-- chunk: 概述 -->## 概述

MongoDB 是全球领先的文档型 NoSQL 数据库，以其灵活的文档模型（BSON/JSON）、丰富的查询语言、水平分片能力和完善的运维工具链，在内容管理、物联网、实时分析、用户画像等领域拥有广泛的企业级部署。MongoDB 8.0 进一步增强了可查询加密（Queryable Encryption）、向量化查询、分片集群自动分片等企业级特性。

企业级 MongoDB 运维需要掌握的核心领域包括：副本集（Replica Set）的高可用配置与故障转移、分片集群（Sharded Cluster）的数据均衡与热点处理、WiredTiger 存储引擎的内存与缓存调优、基于 Oplog 的增量备份与时间点恢复、以及安全加固（SCRAM-SHA-256、TLS、RBAC、审计日志）。本文档系统覆盖上述所有主题，提供生产级配置和脚本。

MongoDB 在 K8s 环境中的运维推荐使用 MongoDB Community Operator（开源）或 MongoDB Atlas（云托管）。对于自建场景，需要特别关注 [[StatefulSet|StatefulSet]] 的有序部署、PodDisruptionBudget 的配置、以及 PVC 的存储类选择。

## MongoDB 技术架构深度解析

MongoDB 的文档模型是其最核心的设计理念。与传统关系型数据库的固定 Schema 不同，MongoDB 使用 BSON（Binary JSON）格式存储数据，支持嵌套文档、数组、多种数据类型（Date、ObjectId、Decimal128、Binary 等）。这种灵活性使得开发者可以在同一个集合中存储结构不同的文档，非常适合快速迭代的互联网应用。然而，灵活性也带来了挑战：缺乏 Schema 约束可能导致数据质量问题，因此生产环境建议使用 JSON Schema Validation 来定义文档结构约束。

WiredTiger 存储引擎是 MongoDB 3.2 以来的默认引擎，它提供了文档级并发控制、多版本并发控制（MVCC）、数据压缩（snappy/zstd/zlib）和 Checkpoint 机制。WiredTiger 的内部缓存（`cacheSizeGB`）独立于 OS 文件缓存，默认使用可用内存的 50% 减去 1GB。理解 WiredTiger 的内存管理机制对于性能调优至关重要：缓存命中率直接影响查询延迟，而过小的缓存会导致频繁的磁盘 I/O。

MongoDB 的复制机制基于 Oplog（操作日志）实现。Primary 节点将所有写操作记录到 Oplog 中，Secondary 节点通过异步拉取 Oplog 并重放来保持数据同步。Oplog 是一个固定大小的 Capped Collection，其大小决定了从库可以离线多长时间后仍然能够自动追赶上来。生产环境建议 Oplog 大小至少能容纳 24-72 小时的写操作量。如果 Oplog 被覆盖，从库需要执行全量重新同步（Initial Sync），这在数据量较大时是一个耗时的操作。

分片集群是 MongoDB 水平扩展的核心机制。通过将数据按照 Shard Key 分布到多个 Shard 上，MongoDB 可以支持 PB 级别的数据存储和百万级的 QPS。Shard Key 的选择是分片集群设计中最重要的决策：好的 Shard Key 应该具有高基数（Cardinality）、低频率（Frequency）和非单调变化（Non-monotonic）三个特征。Hashed Shard Key 适合写入均匀分布的场景，Range Shard Key 适合范围查询场景。对于分片键选择不当的集群，MongoDB 5.0+ 支持在线更改 Shard Key（`reshardCollection`），但这是一个重量级操作，需要在低峰期执行。

---

<!-- chunk: 架构设计 -->## 架构设计

## MongoDB 分片集群架构

```mermaid
graph TB
    subgraph "应用层"
        APP1[应用 1<br/>Connection String]
        APP2[应用 2<br/>Connection String]
    end

    subgraph "路由层"
        MONGOS1[mongos 1<br/>查询路由]
        MONGOS2[mongos 2<br/>查询路由]
        MONGOS3[mongos 3<br/>查询路由]
    end

    subgraph "配置服务器"
        CFG1[Config Server 1<br/>Primary]
        CFG2[Config Server 2<br/>Secondary]
        CFG3[Config Server 3<br/>Secondary]
    end

    subgraph "分片 - Shard 0"
        S0P[Shard 0 Primary]
        S0S1[Shard 0 Secondary 1]
        S0S2[Shard 0 Secondary 2]
    end

    subgraph "分片 - Shard 1"
        S1P[Shard 1 Primary]
        S1S1[Shard 1 Secondary 1]
        S1S2[Shard 1 Secondary 2]
    end

    subgraph "分片 - Shard 2"
        S2P[Shard 2 Primary]
        S2S1[Shard 2 Secondary 2]
        S2S2[Shard 2 Secondary 2]
    end

    APP1 --> MONGOS1
    APP2 --> MONGOS2
    MONGOS1 --> CFG1
    MONGOS2 --> CFG1
    MONGOS3 --> CFG1

    MONGOS1 --> S0P
    MONGOS1 --> S1P
    MONGOS2 --> S1P
    MONGOS2 --> S2P
    MONGOS3 --> S0P
    MONGOS3 --> S2P

    CFG1 --> CFG2
    CFG1 --> CFG3

    S0P --> S0S1
    S0P --> S0S2
    S1P --> S1S1
    S1P --> S1S2
    S2P --> S2S1
    S2P --> S2S2
```

## WiredTiger 存储引擎架构

```mermaid
graph LR
    subgraph "客户端连接"
        C1[Conn 1]
        C2[Conn 2]
    end

    subgraph "WiredTiger 引擎"
        WT_CACHE[WiredTiger Cache<br/>50% RAM - 1GB]
        IDX_CACHE[Index Cache]
        CHECKPOINT[Checkpoint<br/>60s 或 2GB journal]
    end

    subgraph "Journal 系统"
        JOURNAL[Journal Log<br/>WAL 机制]
        JBUF[Journal Buffer<br/>128MB]
    end

    subgraph "磁盘文件"
        DATA_FILES[Collection Files<br/>collection-*.wt]
        IDX_FILES[Index Files<br/>index-*.wt]
        META[Metadata<br/>_mdb_catalog.wt]
    end

    C1 --> WT_CACHE
    C2 --> WT_CACHE
    WT_CACHE --> CHECKPOINT
    CHECKPOINT --> DATA_FILES
    CHECKPOINT --> IDX_FILES
    WT_CACHE --> JBUF
    JBUF --> JOURNAL
    WT_CACHE --> IDX_CACHE
    IDX_CACHE --> IDX_FILES
```

---

<!-- chunk: 核心组件配置 -->## 核心组件配置

## MongoDB 生产配置文件

```yaml
# mongod.conf - MongoDB 8.0 生产配置（Primary 节点）
storage:
  dbPath: /data/mongodb
  engine: wiredTiger
  wiredTiger:
    engineConfig:
      cacheSizeGB: 28
      journalCompressor: zstd
      directoryForIndexes: true
    collectionConfig:
      blockCompressor: zstd
    indexConfig:
      prefixCompression: true
  journal:
    enabled: true
    commitIntervalMs: 100
  directoryPerDB: true

systemLog:
  destination: file
  path: /var/log/mongodb/mongod.log
  logAppend: true
  logRotate: rename
  verbosity: 1
  component:
    command: 1
    network: 1
    storage:
      journal: 1
    replication: 1
    sharding: 1

net:
  port: 27017
  bindIp: "0.0.0.0"
  maxIncomingConnections: 50000
  wireObjectCheck: true
  ipv6: false
  ssl:
    mode: requireSSL
    certificateKeyFile: /etc/ssl/mongodb.pem
    CAFile: /etc/ssl/ca.pem
    clusterCertificateKeyFile: /etc/ssl/cluster.pem
    clusterCAFile: /etc/ssl/ca.pem
    allowConnectionsWithoutCertificates: false
    allowInvalidCertificates: false

security:
  authorization: enabled
  keyFile: /etc/mongodb/keyfile
  clusterAuthMode: keyFile
  javascriptEnabled: false
  redactClientLogData: false
  enableEncryption: true
  encryptionCipherMode: AES256-CBC
  kmip:
    serverName: kmip.company.com
    port: 5696
    clientCertificateFile: /etc/ssl/kmip-client.pem
    serverCAFile: /etc/ssl/kmip-ca.pem

replication:
  oplogSizeMB: 10240
  replSetName: rs0
  enableMajorityReadConcern: true

sharding:
  clusterRole: shardsvr

operationProfiling:
  mode: slowOp
  slowOpThresholdMs: 100
  slowOpSampleRate: 1.0

processManagement:
  fork: true
  pidFilePath: /var/run/mongodb/mongod.pid

setParameter:
  enableLocalhostAuthBypass: false
  logicalSessionRecordCacheRefreshInterval: 300
  maxIndexBuildMemoryUsageMegabytes: 8000
  transactionLifetimeLimitSeconds: 60

auditLog:
  destination: file
  format: JSON
  path: /var/log/mongodb/audit.log
  filter: '{"atype": {"$in": ["authenticate","createCollection","dropCollection","createIndex","dropIndex","createUser","dropUser","grantRole","revokeRole"]}}'
```

## 副本集初始化脚本

```javascript
// rs_init.js - 副本集初始化
rs.initiate({
  _id: "rs0",
  version: 1,
  configsvr: false,
  members: [
    {
      _id: 0,
      host: "mongo-0.mongo-svc.production.svc.cluster.local:27017",
      priority: 10,
      votes: 1,
      tags: { dc: "dc1", usage: "production" }
    },
    {
      _id: 1,
      host: "mongo-1.mongo-svc.production.svc.cluster.local:27017",
      priority: 8,
      votes: 1,
      tags: { dc: "dc1", usage: "production" }
    },
    {
      _id: 2,
      host: "mongo-2.mongo-svc.production.svc.cluster.local:27017",
      priority: 6,
      votes: 1,
      tags: { dc: "dc2", usage: "dr" }
    }
  ],
  settings: {
    chainingAllowed: true,
    heartbeatIntervalMillis: 2000,
    heartbeatTimeoutSecs: 10,
    electionTimeoutMillis: 10000,
    catchUpTimeoutMillis: 30000,
    catchUpTakeoverDelayMillis: 30000,
    getLastErrorModes: {},
    getLastErrorDefaults: { w: 1, wtimeout: 0 }
  }
});

// 等待选举完成
sleep(10000);

// 创建管理员用户
db = db.getSiblingDB("admin");
db.createUser({
  user: "admin",
  pwd: passwordPrompt(),
  roles: [
    { role: "userAdminAnyDatabase", db: "admin" },
    { role: "clusterAdmin", db: "admin" },
    { role: "readWriteAnyDatabase", db: "admin" },
    { role: "dbAdminAnyDatabase", db: "admin" }
  ]
});

// 创建应用用户
db = db.getSiblingDB("appdb");
db.createUser({
  user: "app_user",
  pwd: passwordPrompt(),
  roles: [
    { role: "readWrite", db: "appdb" }
  ]
});

// 创建监控用户
db = db.getSiblingDB("admin");
db.createUser({
  user: "monitoring",
  pwd: passwordPrompt(),
  roles: [
    { role: "clusterMonitor", db: "admin" },
    { role: "read", db: "local" }
  ]
});

// 创建备份用户
db.createUser({
  user: "backup",
  pwd: passwordPrompt(),
  roles: [
    { role: "backup", db: "admin" },
    { role: "clusterAdmin", db: "admin" }
  ]
});
```

## 分片集群管理

```javascript
// sharding_setup.js - 分片集群设置

// 1. 启用数据库分片
sh.enableSharding("appdb");

// 2. 对集合启用分片（Hashed Shard Key）
db.appdb.users.createIndex({ user_id: "hashed" });
sh.shardCollection("appdb.users", { user_id: "hashed" });

// 3. 对集合启用分片（Range Shard Key）
db.appdb.orders.createIndex({ order_date: 1, user_id: 1 });
sh.shardCollection("appdb.orders", { order_date: 1, user_id: 1 });

// 4. 预分片（避免初始热点）
sh.splitAt("appdb.orders", { order_date: ISODate("2026-01-01"), user_id: MinKey });
sh.splitAt("appdb.orders", { order_date: ISODate("2026-04-01"), user_id: MinKey });
sh.splitAt("appdb.orders", { order_date: ISODate("2026-07-01"), user_id: MinKey });

// 5. 配置 Zone 分片（数据本地化）
sh.addShardTag("shard-us-east", "US_EAST");
sh.addShardTag("shard-us-west", "US_WEST");
sh.addShardTag("shard-eu", "EU");

sh.addTagRange("appdb.users", { region: "US_EAST" }, { region: "US_WEST" }, "US_EAST");
sh.addTagRange("appdb.users", { region: "EU" }, { region: "EU~" }, "EU");

// 6. Balancer 管理
sh.getBalancerState();
sh.setBalancerState(true);
sh.startBalancer();
sh.stopBalancer();

// 7. 监控分片状态
sh.status();
db.chunks.find({ ns: "appdb.orders" }).count();
db.chunks.aggregate([
  { $group: { _id: "$shard", chunkCount: { $sum: 1 } } },
  { $sort: { chunkCount: -1 } }
]);
```

---

<!-- chunk: 性能调优 -->## 性能调优

## 内存参数计算

```
MongoDB WiredTiger 内存分配参考（64GB 物理内存）：

cacheSizeGB              = 物理内存 × 50% - 1GB = ~31GB
  （留 50% 给 OS 文件缓存、连接开销、聚合操作）

oplogSizeMB              = 估算: 2小时 oplog 产生量
  （通常 5-20GB，取决于写入速率）

maxIncomingConnections   = 应用连接池总大小 × 1.5 + 管理连接
  （推荐 50000，实际使用 < 5000）

maxIndexBuildMemoryUsageMegabytes = cacheSizeGB × 25% = ~7800MB

journal commitIntervalMs = 100（平衡性能与安全性）
```

## 索引优化实践

```javascript
// 索引诊断与优化

// 1. 创建覆盖查询的复合索引
// ESR 原则: Equality → Sort → Range
db.orders.createIndex(
  { status: 1, created_at: -1, amount: 1 },
  {
    name: "idx_status_created_amount",
    background: true,
    partialFilterExpression: { status: { $in: ["pending", "processing"] } }
  }
);

// 2. 使用 explain 分析查询计划
db.orders.explain("executionStats").find({
  status: "pending",
  created_at: { $gte: ISODate("2026-01-01") }
}).sort({ created_at: -1 }).limit(50);

// 3. 检查索引使用情况
db.orders.aggregate([
  { $indexStats: {} },
  { $sort: { "accesses.ops": -1 } }
]);

// 4. 查找冗余索引
db.getSiblingDB("admin").runCommand({
  validate: "orders",
  full: true
});

// 5. 慢查询分析
db.system.profile.find({
  millis: { $gt: 1000 }
}).sort({ ts: -1 }).limit(20).forEach(doc => {
  print(`${doc.ts} | ${doc.op} | ${doc.millis}ms | ${doc.ns}`);
  print(`  Query: ${JSON.stringify(doc.query || doc.command)}`);
  print(`  KeysExamined: ${doc.keysExamined}, DocsExamined: ${doc.docsExamined}`);
  print(`  IndexUsed: ${doc.planSummary}`);
});
```

## 查询性能优化

```javascript
// 优化前：全表扫描
// 扫描 100 万行，返回 10 行
db.users.find({ email: { $regex: "@company.com$" } });

// 优化后：使用精确匹配索引
db.users.createIndex({ email: 1 });
db.users.find({ email: { $regex: "^user123@company.com$" } });

// 优化批量写入
// 差：循环 insert
for (let i = 0; i < 10000; i++) {
  db.logs.insertOne({ msg: `log ${i}`, ts: new Date() });
}

// 好：bulkWrite 无序批量
const bulkOps = [];
for (let i = 0; i < 100000; i++) {
  bulkOps.push({
    insertOne: { document: { msg: `log ${i}`, ts: new Date() } }
  });
}
db.logs.bulkWrite(bulkOps, { ordered: false });

// 使用 readPreference 分担读负载
// 主库读写
db.orders.find({ status: "pending" }).readPref("primary");

// 从库读取（可容忍最终一致性）
db.reports.find({ type: "daily" }).readPref("secondaryPreferred");

// 按 Tag 读取（就近读取）
db.users.find({}).readPref("nearest", [
  { dc: "dc1" }
]);
```

---

<!-- chunk: 高可用与容灾 -->## 高可用与容灾

## 跨机房副本集配置

```javascript
// 跨机房副本集配置（3 节点 + 1 仲裁者 + 1 延迟节点）
cfg = rs.conf();
cfg.members = [
  {
    _id: 0,
    host: "mongo-primary.dc1:27017",
    priority: 10,
    votes: 1,
    tags: { dc: "dc1", role: "primary" }
  },
  {
    _id: 1,
    host: "mongo-secondary.dc1:27017",
    priority: 8,
    votes: 1,
    tags: { dc: "dc1", role: "secondary" }
  },
  {
    _id: 2,
    host: "mongo-secondary.dc2:27017",
    priority: 6,
    votes: 1,
    tags: { dc: "dc2", role: "secondary" }
  },
  {
    _id: 3,
    host: "mongo-arbiter.dc1:27017",
    arbiterOnly: true,
    votes: 1
  },
  {
    _id: 4,
    host: "mongo-delayed.dc2:27017",
    priority: 0,
    votes: 0,
    hidden: true,
    slaveDelay: 3600,
    tags: { dc: "dc2", role: "delayed_backup" }
  }
];
cfg.settings = {
  chainingAllowed: true,
  heartbeatTimeoutSecs: 10,
  electionTimeoutMillis: 10000,
  getLastErrorModes: {
    datacenter: { dc: 2 }
  }
};
rs.reconfig(cfg);
```

## 故障转移测试

```bash
#!/bin/bash
# mongo_failover_test.sh - MongoDB 故障转移测试脚本

REPLICA_SET="rs0"
PRIMARY_HOST=$(mongo --quiet --eval "rs.isMaster().primary" | tr -d '"')
echo "Current Primary: $PRIMARY_HOST"

echo "Step 1: Simulating primary failure..."
# 方法1：使用 rs.stepDown
mongo --host "$PRIMARY_HOST" --eval "rs.stepDown(120, 30)"

echo "Step 2: Waiting for election..."
sleep 15

NEW_PRIMARY=$(mongo --quiet --eval "rs.isMaster().primary" | tr -d '"')
echo "New Primary: $NEW_PRIMARY"

if "$PRIMARY_HOST" != "$NEW_PRIMARY"; then
    echo "FAILOVER SUCCESS: $PRIMARY_HOST -> $NEW_PRIMARY"
else
    echo "FAILOVER FAILED: Primary unchanged"
    exit 1
fi

echo "Step 3: Verifying cluster health..."
mongo --eval "
  var status = rs.status();
  status.members.forEach(function(m) {
    print(m.name + ' -> ' + m.stateStr + ' (health: ' + m.health + ')');
  });
"

echo "Step 4: Verifying write capability..."
mongo --eval "
  var result = db.getSiblingDB('admin').runCommand({ ping: 1 });
  print('Write test: ' + (result.ok === 1 ? 'PASSED' : 'FAILED'));
"
```

---

<!-- chunk: 备份恢复 -->## 备份恢复

## 生产级备份脚本

```bash
#!/bin/bash
# mongodb_backup.sh - MongoDB 综合备份方案
set -euo pipefail

BACKUP_ROOT="/backup/mongodb"
DATE=$(date +%Y%m%d_%H%M%S)
MONGO_URI="mongodb://backup:${BACKUP_PASSWORD}@mongo-primary:27017/admin?replicaSet=rs0"
RETENTION_DAYS=14
S3_BUCKET="s3://company-mongodb-backup"

logical_backup() {
    local backup_dir="${BACKUP_ROOT}/logical_${DATE}"
    mkdir -p "$backup_dir"

    echo "$(date): Starting logical backup..."
    mongodump \
        --uri="${MONGO_URI}" \
        --out="${backup_dir}" \
        --gzip \
        --oplog \
        --numParallelCollections=4 \
        --verbosity=1

    md5sum "${backup_dir}"/* > "${backup_dir}.md5"

    aws s3 sync "${backup_dir}" "${S3_BUCKET}/logical_${DATE}/" \
        --storage-class STANDARD_IA --no-progress

    echo "$(date): Logical backup completed: ${backup_dir}"
}

snapshot_backup() {
    echo "$(date): Starting snapshot backup..."

    local volume_id=$(aws ec2 describe-volumes \
        --filters "Name=tag:Name,Values=mongodb-data" "Name=state,Values=available" \
        --query 'Volumes[0].VolumeId' --output text)

    aws ec2 create-snapshot \
        --volume-id "$volume_id" \
        --description "MongoDB snapshot ${DATE}" \
        --tag-specifications "ResourceType=snapshot,Tags=[{Key=Type,Value=mongodb},{Key=Date,Value=${DATE}}]"

    echo "$(date): Snapshot backup completed"
}

oplog_backup() {
    echo "$(date): Starting oplog incremental backup..."
    local backup_file="${BACKUP_ROOT}/oplog_${DATE}.bson.gz"

    mongodump \
        --uri="${MONGO_URI}" \
        --collection=oplog.rs \
        --db=local \
        --query="{ ts: { \$gt: Timestamp($(cat ${BACKUP_ROOT}/last_oplog_ts 2>/dev/null || echo '0,0')) } }" \
        --gzip \
        --out="${backup_file}"

    mongo --quiet --eval "
      var latest = db.getSiblingDB('local').oplog.rs.find().sort({ts:-1}).limit(1).next();
      print(Math.floor(latest.ts.getTime()/1000) + ',' + latest.ts.getInc());
    " > "${BACKUP_ROOT}/last_oplog_ts"

    aws s3 cp "${backup_file}" "${S3_BUCKET}/oplogs/oplog_${DATE}.bson.gz"
    echo "$(date): Oplog backup completed"
}

restore_logical() {
    local backup_path="${1:?Backup path required}"
    local target_uri="${2:?Target MongoDB URI required}"

    echo "!!! RESTORING from $backup_path !!!"
    read -p "Confirm? (yes/no): " confirm
    "$confirm" != "yes" && exit 0

    mongorestore \
        --uri="${target_uri}" \
        --gzip \
        --oplogReplay \
        --drop \
        --numParallelCollections=4 \
        "$backup_path"

    echo "Restore completed"
}

cleanup() {
    find "${BACKUP_ROOT}" -name "logical_*" -type d -mtime +${RETENTION_DAYS} -exec rm -rf {} \;
    find "${BACKUP_ROOT}" -name "oplog_*" -mtime +7 -delete
}

case "${1:-help}" in
    logical)  logical_backup ;;
    snapshot) snapshot_backup ;;
    oplog)    oplog_backup ;;
    restore)  restore_logical "${2:?}" "${3:?}" ;;
    cleanup)  cleanup ;;
    all)      logical_backup && oplog_backup && cleanup ;;
    *)        echo "Usage: $0 {logical|snapshot|oplog|restore|cleanup|all}" ;;
esac
```

---

<!-- chunk: 监控告警 -->## 监控告警

## Prometheus 告警规则

```yaml
groups:
  - name: mongodb.rules
    rules:
      - alert: MongoDBDown
        expr: mongodb_up == 0
        for: 1m
        labels:
          severity: critical
          team: dba
        annotations:
          summary: "MongoDB 实例宕机"
          description: "实例 {{ $labels.instance }} 不可达"

      - alert: MongoDBReplicationLag
        expr: mongodb_replication_lag > 30
        for: 5m
        labels:
          severity: warning
          team: dba
        annotations:
          summary: "MongoDB 复制延迟超过 30 秒"

      - alert: MongoDBHighConnections
        expr: mongodb_connections / mongodb_connections_available > 0.85
        for: 5m
        labels:
          severity: warning
          team: dba
        annotations:
          summary: "MongoDB 连接使用率超过 85%"

      - alert: MongoDBMemoryUsage
        expr: mongodb_wiredtiger_cache_bytes / mongodb_wiredtiger_cache_max_bytes > 0.9
        for: 10m
        labels:
          severity: warning
          team: dba
        annotations:
          summary: "WiredTiger 缓存使用率超过 90%"

      - alert: MongoDBReplicaSetNoPrimary
        expr: count(mongodb_rs_member_state == 1) by (cluster) == 0
        for: 2m
        labels:
          severity: critical
          team: dba
        annotations:
          summary: "副本集没有 Primary 节点"

      - alert: MongoDBOplogWindowTooShort
        expr: mongodb_oplog_window < 3600
        for: 10m
        labels:
          severity: warning
          team: dba
        annotations:
          summary: "Oplog 时间窗口不足 1 小时"

      - alert: MongoDBSlowQueries
        expr: rate(mongodb_slow_queries[5m]) > 10
        for: 5m
        labels:
          severity: warning
          team: dba
        annotations:
          summary: "慢查询频率超过 10/min"

      - alert: MongoDBPageFaults
        expr: rate(mongodb_page_faults[5m]) > 100
        for: 10m
        labels:
          severity: warning
          team: dba
        annotations:
          summary: "Page fault 频率过高，可能内存不足"
```

---

<!-- chunk: 运维管理 -->## 运维管理

## 综合运维脚本

```bash
#!/bin/bash
# mongodb_ops.sh - MongoDB 运维管理脚本
set -euo pipefail

MONGO="mongo --quiet"

cmd_health() {
    echo "=== MongoDB Health Check $(date) ==="

    echo ""
    echo "--- Replica Set Status ---"
    $MONGO --eval '
      var s = rs.status();
      print("Set: " + s.set);
      print("Date: " + s.date);
      s.members.forEach(function(m) {
        print(m.name + " -> " + m.stateStr +
              " | health=" + m.health +
              " | pingMs=" + (m.pingMs || "N/A") +
              " | optimeDate=" + m.optimeDate.toISOString());
      });
    '

    echo ""
    echo "--- Connection Stats ---"
    $MONGO --eval '
      var s = db.serverStatus().connections;
      print("Current: " + s.current + " / Available: " + s.available);
    '

    echo ""
    echo "--- Memory Usage ---"
    $MONGO --eval '
      var s = db.serverStatus().wiredTiger.cache;
      print("Cache used: " + (s.bytesCurrentlyInCache / 1024/1024/1024).toFixed(2) + " GB");
      print("Cache max: " + (s.maximumBytesConfigured / 1024/1024/1024).toFixed(2) + " GB");
      print("Evictions: " + s.eviction.caller+ " (caller), " + s.eviction.server + " (server)");
    '

    echo ""
    echo "--- Oplog Window ---"
    $MONGO --eval '
      var first = db.getSiblingDB("local").oplog.rs.find().sort({ts:1}).limit(1).next().ts;
      var last = db.getSiblingDB("local").oplog.rs.find().sort({ts:-1}).limit(1).next().ts;
      var hours = (last.getTime() - first.getTime()) / 3600000;
      print("Oplog window: " + hours.toFixed(1) + " hours");
      print("Oplog size: " + (db.getSiblingDB("local").oplog.rs.stats().size / 1024/1024/1024).toFixed(2) + " GB");
    '

    echo ""
    echo "--- Database Sizes ---"
    $MONGO --eval '
      db.adminCommand("listDatabases").databases.forEach(function(d) {
        if (d.sizeOnDisk > 0) {
          print(d.name + ": " + (d.sizeOnDisk / 1024/1024/1024).toFixed(2) + " GB");
        }
      });
    '
}

cmd_index_audit() {
    echo "=== Index Audit ==="
    $MONGO --eval '
      db.adminCommand("listDatabases").databases.forEach(function(d) {
        if (d.name !== "admin" && d.name !== "local" && d.name !== "config") {
          var dbObj = db.getSiblingDB(d.name);
          dbObj.getCollectionNames().forEach(function(c) {
            var stats = dbObj.runCommand({ collStats: c });
            var indexes = dbObj[c].getIndexes();
            indexes.forEach(function(idx) {
              if (idx.name !== "_id_") {
                print(d.name + "." + c + " | " + idx.name + " | " + JSON.stringify(idx.key));
              }
            });
          });
        }
      });
    '
}

case "${1:-help}" in
    health)  cmd_health ;;
    indexes) cmd_index_audit ;;
    *)       echo "Usage: $0 {health|indexes}" ;;
esac
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

## 1. 文档建模原则

- 嵌入 vs 引用：一对一和一对少量关系嵌入，一对多大量关系引用
- 文档大小限制 16MB，实际建议 < 100KB
- 使用 `db.collection.stats()` 监控平均文档大小
- 避免无限增长的数组字段

## 2. 索引设计 ESR 原则

```
复合索引字段顺序: Equality → Sort → Range

示例查询:
  db.orders.find({ status: "pending" }).sort({ created_at: -1 }).limit(50)

最优索引:
  db.orders.createIndex({ status: 1, created_at: -1 })

解释:
  - status: 等值过滤 (Equality)
  - created_at: 排序 (Sort)
  - 无 Range 字段
```

## 3. 分片键选择

| 好的分片键 | 差的分片键 |
|:---|:---|
| `user_id` (hashed) 高基数 | `status` 低基数 |
| `region` + `_id` (compound) | 自增 `_id` (热点) |
| `tenant_id` (多租户) | 随机 UUID (索引膨胀) |

---

<!-- chunk: 故障排查 -->## 故障排查

## 常见问题速查表

| 问题现象 | 可能原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| `ExceededTimeLimit` | 慢查询/锁等待 | `db.currentOp()` 查看长操作 | 添加索引、优化查询、设置 `maxTimeMS` |
| `WriteConflict` | 并发写冲突（WiredTiger） | 查看慢查询日志 | 重试机制、减少并发写同一文档 |
| ` ReplicaSetNoPrimary` | 选举失败/网络分区 | `rs.status()` 检查成员状态 | 检查网络、确保多数派可达 |
| Oplog 耗尽 | 从库长时间离线 | 检查 oplog window | 增大 oplogSize / 全量 resync |
| 内存持续增长 | 未设置 cacheSizeGB | `db.serverStatus().wiredTiger.cache` | 显式设置 `cacheSizeGB` |
| `Cursor not found` | 游标超时 | 检查 `cursorTimeoutMillis` | 增大超时或使用 `noCursorTimeout` |
| 分片不均衡 | balancer 未运行 | `sh.getBalancerState()` | 启动 balancer、检查 chunk 大小 |
| `Too many open files` | OS ulimit 限制 | `ulimit -n` | 设置 `nofile 65535` |

---

**文档版本**: v2.0  
**最后更新**: 2026-04-26  
**适用版本**: MongoDB 7.0 ~ 8.0

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-28-enterprise-database-middleware MOC
- [[domain-16-database-middleware/README.md|Domain 16: 企业级数据库与中间件运维 (Enterprise Database & Middleware Op...]]
- Domain-28 企业数据库与中间件 — 开源项目索引
- MySQL 企业级数据库运维管理
- PostgreSQL 企业级数据库高可用架构
- 分布式数据库企业级实践深度指南
- 数据库中间件 Kubernetes 企业级实践
- Redis 企业级缓存运维深度实践
- Redis Kubernetes Operator 企业级实践
- Kafka Kubernetes 企业级实践 — Strimzi Operator 深度指南
- CloudNativePG 企业级 PostgreSQL 运维指南

## See Also

- 03-distributed-database-enterprise
- 04-database-middleware-kubernetes
- 06-redis-enterprise-cache
- 07-redis-kubernetes-operator
