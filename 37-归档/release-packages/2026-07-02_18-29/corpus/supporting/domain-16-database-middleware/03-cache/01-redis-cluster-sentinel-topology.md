---
title: Redis 集群架构与高可用
description: 'Redis Cluster 槽位分配、Sentinel 高可用、数据分片、内存优化、持久化与 Operator 选型'
summary: 'Redis Cluster 槽位分配、Sentinel 高可用、数据分片、内存优化、持久化与 Operator 选型'
category: database-middleware
tags:
- database
- k8s
- redis
- cache
- cluster
- sentinel
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DBA
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Redis 集群架构与高可用 是什么
- 如何 Redis 集群架构与高可用
trigger_keywords:
- redis
- cluster
- sentinel
- 槽位
- 持久化
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


# Redis 集群架构与高可用

## 1. 架构模式选型

### 1.1 三种模式对比

| 特性 | 单机 | Sentinel | Cluster |
|------|------|----------|---------|
| 高可用 | 无 | 自动故障转移 | 自动故障转移 |
| 数据分片 | 不支持 | 不支持 | 支持 16384 槽位 |
| 最大容量 | 单实例 | 单主容量 | 水平扩展 |
| 读扩展 | 不支持 | 从节点读 | 从节点读 |
| 多键操作 | 支持 | 支持 | 需同 slot |
| 复杂度 | 低 | 中 | 高 |
| 推荐场景 | 开发测试 | 中小规模 | 大规模生产 |

### 1.2 生产选型建议

```
QPS < 10 万 + 数据量 < 16GB → Sentinel (1主2从)
QPS > 10 万 或 数据量 > 16GB → Cluster (3主3从起步)
跨区域高可用 → Cluster + 跨 DC 副本
```

## 2. Redis Cluster 部署

### 2.1 6 节点集群 (3主3从)

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-cluster
  namespace: cache
spec:
  serviceName: redis-cluster-headless
  replicas: 6
  selector:
    matchLabels:
      app: redis-cluster
  template:
    metadata:
      labels:
        app: redis-cluster
    spec:
      containers:
      - name: redis
        image: redis:7.2.4
        ports:
        - containerPort: 6379
          name: redis
        - containerPort: 16379
          name: gossip
        command:
        - bash
        - -c
        - |
          # 获取 Pod 序号
          ORDINAL=${HOSTNAME##*-}

          # 生成配置
          cat > /data/redis.conf <<EOF
          port 6379
          cluster-enabled yes
          cluster-config-file nodes.conf
          cluster-node-timeout 5000
          cluster-announce-hostname ${HOSTNAME}.redis-cluster-headless.cache.svc.cluster.local
          cluster-announce-port 6379
          cluster-announce-bus-port 16379

          appendonly yes
          appendfsync everysec
          auto-aof-rewrite-percentage 100
          auto-aof-rewrite-min-size 64mb

          maxmemory 8gb
          maxmemory-policy allkeys-lru

          tcp-keepalive 300
          timeout 0
          tcp-backlog 511

          save 900 1
          save 300 10
          save 60 10000

          slowlog-log-slower-than 10000
          slowlog-max-len 128

          lua-time-limit 5000

          hz 10
          dynamic-hz yes
          EOF

          exec redis-server /data/redis.conf
        resources:
          requests:
            cpu: "2"
            memory: 8Gi
          limits:
            cpu: "4"
            memory: 10Gi
        volumeMounts:
        - name: data
          mountPath: /data
        livenessProbe:
          tcpSocket:
            port: redis
          initialDelaySeconds: 15
          periodSeconds: 10
        readinessProbe:
          exec:
            command: ["redis-cli", "ping"]
          initialDelaySeconds: 5
          periodSeconds: 5
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3
      resources:
        requests:
          storage: 20Gi
---
apiVersion: v1
kind: Service
metadata:
  name: redis-cluster-headless
  namespace: cache
spec:
  clusterIP: None
  ports:
  - port: 6379
    name: redis
  - port: 16379
    name: gossip
  selector:
    app: redis-cluster
```

### 2.2 集群初始化 Job

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: redis-cluster-init
  namespace: cache
spec:
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: init
        image: redis:7.2.4
        command:
        - bash
        - -c
        - |
          # 等待所有 Pod 就绪
          for i in $(seq 0 5); do
            until redis-cli -h redis-cluster-${i}.redis-cluster-headless -p 6379 ping; do
              echo "Waiting for redis-cluster-${i}..."
              sleep 2
            done
          done

          # 创建集群 (3主3从)
          redis-cli --cluster create \
            redis-cluster-0.redis-cluster-headless:6379 \
            redis-cluster-1.redis-cluster-headless:6379 \
            redis-cluster-2.redis-cluster-headless:6379 \
            redis-cluster-3.redis-cluster-headless:6379 \
            redis-cluster-4.redis-cluster-headless:6379 \
            redis-cluster-5.redis-cluster-headless:6379 \
            --cluster-replicas 1 \
            --cluster-yes

          # 验证集群
          redis-cli -c -h redis-cluster-0.redis-cluster-headless cluster info
          redis-cli -c -h redis-cluster-0.redis-cluster-headless cluster nodes
```

### 2.3 16384 槽位分配

```
┌─────────────────────────────────────────────────────────────────┐
│                    Redis Cluster 槽位分配                        │
│                                                                 │
│  Master 0 (redis-cluster-0): slots 0    - 5460  (5461 slots)   │
│  Master 1 (redis-cluster-1): slots 5461 - 10922 (5462 slots)   │
│  Master 2 (redis-cluster-2): slots 10923- 16383 (5461 slots)   │
│                                                                 │
│  Slave 3 (redis-cluster-3):  replicates Master 0                │
│  Slave 4 (redis-cluster-4):  replicates Master 1                │
│  Slave 5 (redis-cluster-5):  replicates Master 2                │
│                                                                 │
│  槽位计算: CRC16(key) % 16384                                   │
│  Hash Tag: {user}:profile 和 {user}:settings 同 slot            │
└─────────────────────────────────────────────────────────────────┘
```

## 3. Sentinel 高可用

### 3.1 Sentinel 部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-sentinel
  namespace: cache
spec:
  serviceName: redis-sentinel-headless
  replicas: 3
  selector:
    matchLabels:
      app: redis-sentinel
  template:
    metadata:
      labels:
        app: redis-sentinel
    spec:
      containers:
      - name: sentinel
        image: redis:7.2.4
        ports:
        - containerPort: 26379
          name: sentinel
        command:
        - bash
        - -c
        - |
          cat > /data/sentinel.conf <<EOF
          port 26379
          sentinel monitor mymaster redis-master-0.redis-master-headless.cache.svc.cluster.local 6379 2
          sentinel down-after-milliseconds mymaster 5000
          sentinel failover-timeout mymaster 30000
          sentinel parallel-syncs mymaster 1
          sentinel resolve-hostnames yes
          sentinel announce-hostnames yes
          EOF

          exec redis-sentinel /data/sentinel.conf
        resources:
          requests:
            cpu: 250m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        volumeMounts:
        - name: data
          mountPath: /data
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3
      resources:
        requests:
          storage: 1Gi
```

### 3.2 Sentinel 故障转移流程

```
正常状态:
  Sentinel 1 ←──monitor──→ Sentinel 2 ←──monitor──→ Sentinel 3
       │                        │                        │
       ▼                        ▼                        ▼
  ┌─────────┐            ┌─────────┐            ┌─────────┐
  │ Master  │◄──replicate─│ Slave 1 │◄──replicate─│ Slave 2 │
  └─────────┘            └─────────┘            └─────────┘

Master 故障后:
  1. Sentinel 检测到 Master 主观下线 (SDOWN)
  2. Sentinel 之间投票确认客观下线 (ODOWN)
  3. Raft 选举 Leader Sentinel
  4. Leader 选择最优 Slave 提升为新 Master
     (优先级最低 → 复制偏移量最大 → runid 最小)
  5. 其余 Slave 重新指向新 Master
  6. 旧 Master 恢复后自动成为 Slave
```

## 4. 数据分片策略

### 4.1 Hash Tag 控制分片

```bash
# 不使用 Hash Tag - 数据分散在不同 slot
SET user:1001:name "Alice"     # 可能落在 slot 8106
SET user:1001:email "a@b.com"  # 可能落在 slot 2917

# 使用 Hash Tag - 相同前缀的数据落在同一 slot
SET {user:1001}:name "Alice"   # CRC16("user:1001") % 16384
SET {user:1001}:email "a@b.com"  # 同一 slot

# 多键操作只在同 slot 内有效
MGET {user:1001}:name {user:1001}:email  # OK
MGET {user:1001}:name {user:1002}:name   # CROSSSLOT 错误
```

### 4.2 分片容量规划

```
单实例内存上限: 建议不超过 10GB（避免 RDB fork 阻塞）
集群总容量 = Master 数 × 单实例内存
Master 数 = ceil(总数据量 / 单实例内存)
Slave 数 = Master 数 (1:1 配比)
总节点数 = Master + Slave

示例:
  总数据量 50GB → 6 Master × 10GB + 6 Slave = 12 节点
  QPS 需求 30 万 → 每节点约 5 万 QPS，需验证
```

## 5. 内存优化

### 5.1 maxmemory-policy 策略

| 策略 | 行为 | 适用场景 |
|------|------|---------|
| noeviction | 内存满时拒绝写入 | 不允许丢失数据 |
| allkeys-lru | 所有键中淘汰最近最少使用 | 通用缓存（推荐） |
| allkeys-lfu | 所有键中淘汰最不常用 | 热点数据明显 |
| volatile-lru | 有过期时间的键中 LRU 混合缓存 | 部分数据需持久 |
| volatile-lfu | 有过期时间的键中 LFU | TTL 键 + 热点识别 |
| volatile-ttl | 淘汰最快过期的键 | 按优先级设置 TTL |
| volatile-random | 随机淘汰有过期时间的键 | 不关心淘汰顺序 |

### 5.2 内存优化配置

```redis
# 内存策略
maxmemory 8gb
maxmemory-policy allkeys-lru
maxmemory-samples 10

# 对象编码优化
hash-max-ziplist-entries 512
hash-max-ziplist-value 64
list-max-ziplist-size -2
list-compress-depth 1
set-max-intset-entries 512
zset-max-ziplist-entries 128
zset-max-ziplist-value 64
stream-node-max-entries 100
stream-node-max-bytes 4096

# 惰性删除
lazyfree-lazy-eviction yes
lazyfree-lazy-expire yes
lazyfree-lazy-server-del yes
replica-lazy-flush yes
lazyfree-lazy-user-del yes
lazyfree-lazy-user-flush no
```

### 5.3 内存分析

```bash
# 查看内存使用详情
redis-cli info memory

# 大 Key 扫描
redis-cli --bigkeys

# 内存分析 (Redis 7.0+)
redis-cli memory doctor
redis-cli memory usage <key>

# 内存碎片率
redis-cli info memory | grep mem_fragmentation_ratio
# 碎片率 > 1.5 需要关注
```

## 6. 持久化策略

### 6.1 RDB vs AOF

| 特性 | RDB | AOF |
|------|-----|-----|
| 持久化方式 | 定时快照 | 追加写命令 |
| 数据安全性 | 可能丢失几分钟 | 最多丢失 1 秒 |
| 恢复速度 | 快 | 慢 |
| 文件大小 | 小（压缩） | 大（可 rewrite） |
| 性能影响 | fork 时有阻塞 | 持续写入 |
| 推荐配置 | 备份用 | 主持久化方式 |

### 6.2 生产持久化配置

```redis
# AOF 为主持久化
appendonly yes
appendfsync everysec
auto-aof-rewrite-percentage 100
auto-aof-rewrite-min-size 64mb
aof-use-rdb-preamble yes

# RDB 为备份
save 900 1
save 300 10
save 60 10000
rdbcompression yes
rdbchecksum yes

# 大实例避免 fork 阻塞
# 如果内存 > 12GB，考虑关闭 RDB 只用 AOF
# save ""
```

### 6.3 备份策略

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# redis-backup.sh
REDIS_HOST="redis-cluster-0.redis-cluster-headless"
BACKUP_DIR="/backup/redis/$(date +%Y%m%d)"
mkdir -p ${BACKUP_DIR}

# 触发 RDB 快照
redis-cli -h ${REDIS_HOST} BGSAVE

# 等待完成
while [ "$(redis-cli -h ${REDIS_HOST} LASTSAVE)" == "${LAST_SAVE}" ]; do
  sleep 1
done

# 复制 RDB 文件
kubectl exec -n cache redis-cluster-0 -- \
  cat /data/dump.rdb > ${BACKUP_DIR}/dump.rdb

# 上传到 S3
aws s3 sync ${BACKUP_DIR} s3://redis-backups/$(date +%Y%m%d)/

# 保留 7 天
find /backup/redis -type d -mtime +7 -exec rm -rf {} +
```
## 7. Redis Operator 选型

### 7.1 Operator 对比

| Operator | 维护者 | Cluster 支持 | Sentinel 支持 | 特点 |
|----------|--------|-------------|-------------|------|
| Redis Operator (OT-CONTAINER) | Opstree | 支持 | 支持 | 轻量，社区活跃 |
| Redis Enterprise Operator | Redis Ltd. | 支持 | - | 企业版功能 |
| Spotahome Redis Operator | Spotahome | - | 支持 | 专注 Sentinel |
| KubeDB | AppsCode | 支持 | 支持 | 多数据库统一管理 |

### 7.2 OT-Container Redis Operator

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装
helm repo add ot-helm https://ot-container-kit.github.io/helm-charts/
helm install redis-operator ot-helm/redis-operator \
  -n redis --create-namespace

# Redis Cluster CR
cat <<EOF | kubectl apply -f -
apiVersion: redis.redis.opstreelabs.in/v1beta2
kind: RedisCluster
metadata:
  name: prod-redis
  namespace: cache
spec:
  clusterSize: 6
  clusterVersion: v7
  persistenceEnabled: true
  image: redis:7.2.4
  kubernetesConfig:
    image: redis:7.2.4
    imagePullPolicy: IfNotPresent
    resources:
      requests:
        cpu: "2"
        memory: 8Gi
      limits:
        cpu: "4"
        memory: 10Gi
  redisLeader:
    replicas: 3
    serviceType: ClusterIP
  redisFollower:
    replicas: 3
    serviceType: ClusterIP
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: gp3
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 20Gi
  redisExporter:
    enabled: true
    image: oliver006/redis_exporter:v1.58.0
EOF
```
## 8. 监控告警

### 8.1 关键指标

| 指标 | 命令/指标 | 告警阈值 |
|------|----------|---------|
| 内存使用率 | `used_memory / maxmemory` | > 80% |
| 命中率 | `keyspace_hits / (hits+misses)` | < 90% |
| 连接数 | `connected_clients` | > maxclients * 0.8 |
| 淘汰数 | `evicted_keys` | 持续增长 |
| 延迟 | `redis-cli --latency` | > 1ms |
| 慢查询 | `slowlog get` | 频繁出现 |
| 主从延迟 | `master_repl_offset - slave_repl_offset` | > 1MB |
| 内存碎片率 | `mem_fragmentation_ratio` | > 1.5 |

### 8.2 Prometheus 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: redis-alerts
  namespace: monitoring
spec:
  groups:
  - name: redis
    rules:
    - alert: RedisMemoryHigh
      expr: |
        redis_memory_used_bytes / redis_memory_max_bytes > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Redis 内存使用率超过 80%"
    - alert: RedisHighLatency
      expr: redis_commands_duration_seconds_total / redis_commands_total > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Redis 命令平均延迟超过 10ms"
    - alert: RedisClusterNodeDown
      expr: redis_cluster_state != 1
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Redis Cluster 状态异常"
    - alert: RedisReplicationBroken
      expr: redis_connected_slaves < 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Redis 主节点无从节点连接"
```

## 9. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 集群状态 fail | `redis-cli cluster info` | 节点不可达、槽位未覆盖 |
| 写入 OOM | `redis-cli info memory` | maxmemory-policy= noeviction |
| 大 Key 阻塞 | `redis-cli --bigkeys` | 未合理设计数据结构 |
| 主从不同步 | `redis-cli info replication` | 网络延迟、repl-backlog 不足 |
| 连接耗尽 | `redis-cli info clients` | 连接池配置不当 |
| 慢查询 | `redis-cli slowlog get 10` | O(N) 命令、大 Key 操作 |


<!-- risk-assessed -->
