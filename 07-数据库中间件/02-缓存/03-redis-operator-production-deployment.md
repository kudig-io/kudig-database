---
title: Redis Operator Production Deployment — Redis Cluster/Sentinel on Kubernetes
description: K8s 上 Redis 生产部署 — Redis Operator、Cluster 模式、Sentinel 高可用、持久化、性能调优、故障转移
summary: 使用 Redis Operator 在 Kubernetes 上运行生产级 Redis 集群的完整实践指南
category: practice
tags:
- redis
- operator
- caching
- high-availability
- persistence
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: database
---
# Redis Operator 生产部署实践

> 在 Kubernetes 上使用 Operator 管理 Redis 集群的完整生产指南。

## 架构选型

| 模式 | 适用场景 | 数据量 | 可用性 | 复杂度 |
|------|----------|--------|--------|--------|
| Standalone | 开发/缓存可丢失 | < 1GB | 低 | 低 |
| Sentinel | 中小规模高可用 | < 16GB | 高 | 中 |
| Cluster | 大规模分片 | > 16GB | 高 | 高 |
| Redis Operator | 全托管生命周期 | 任意 | 最高 | 中 |

## Redis Operator 部署（OT-Helm/Spotahome）

### 安装 Operator

```bash
# 使用 OT-Helm Redis Operator（推荐）
helm repo add ot-helm https://ot-container-kit.github.io/helm-charts/
helm install redis-operator ot-helm/redis-operator \
  --namespace redis-system --create-namespace \
  --set resources.requests.cpu=100m \
  --set resources.requests.memory=128Mi
```

### Redis Cluster 部署

```yaml
apiVersion: redis.redis.opstreelabs.in/v1beta2
kind: RedisCluster
metadata:
  name: redis-cluster
  namespace: production
spec:
  clusterSize: 6  # 3 master + 3 replica
  clusterVersion: v7
  persistenceEnabled: true
  kubernetesConfig:
    image: quay.io/opstree/redis:v7.2.4
    imagePullPolicy: IfNotPresent
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
      limits:
        cpu: "2"
        memory: 4Gi
  redisLeader:
    replicas: 3
    redisConfig:
      additionalRedisConfig: |
        maxmemory-policy allkeys-lru
        maxmemory 3gb
        save 900 1
        save 300 10
        save 60 10000
        tcp-keepalive 300
        timeout 0
    affinity:
      podAntiAffinity:
        requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: redis-cluster-leader
            topologyKey: kubernetes.io/hostname
  redisFollower:
    replicas: 3
    redisConfig:
      additionalRedisConfig: |
        maxmemory-policy allkeys-lru
        maxmemory 3gb
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: gp3-encrypted
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 20Gi
  podSecurityContext:
    runAsUser: 1000
    fsGroup: 1000
  priorityClassName: high-priority
  redisExporter:
    enabled: true
    image: quay.io/opstree/redis-exporter:v1.0.1
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
```

### Redis Sentinel 部署

```yaml
apiVersion: redis.redis.opstreelabs.in/v1beta2
kind: RedisSentinel
metadata:
  name: redis-sentinel
  namespace: production
spec:
  clusterSize: 3
  redisSentinelConfig:
    redisReplicationName: redis-replication
    additionalSentinelConfig: |
      sentinel down-after-milliseconds mymaster 5000
      sentinel failover-timeout mymaster 60000
      sentinel parallel-syncs mymaster 1
  kubernetesConfig:
    image: quay.io/opstree/redis-sentinel:v7.2.4
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
---
apiVersion: redis.redis.opstreelabs.in/v1beta2
kind: RedisReplication
metadata:
  name: redis-replication
  namespace: production
spec:
  clusterSize: 3
  kubernetesConfig:
    image: quay.io/opstree/redis:v7.2.4
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: gp3-encrypted
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 10Gi
  redisConfig:
    additionalRedisConfig: |
      maxmemory 2gb
      maxmemory-policy volatile-lru
```

## 应用连接配置

### Sentinel 模式连接

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: redis-config
  namespace: production
data:
  REDIS_MODE: sentinel
  REDIS_SENTINEL_HOSTS: "redis-sentinel-0.redis-sentinel-headless:26379,redis-sentinel-1.redis-sentinel-headless:26379,redis-sentinel-2.redis-sentinel-headless:26379"
  REDIS_SENTINEL_MASTER: mymaster
  REDIS_DB: "0"
  REDIS_TIMEOUT: "3000"
  REDIS_POOL_SIZE: "50"
  REDIS_MIN_IDLE: "10"
```

### Node.js 客户端（ioredis）

```javascript
const Redis = require('ioredis');

// Sentinel 模式
const redis = new Redis({
  sentinels: [
    { host: 'redis-sentinel-0.redis-sentinel-headless', port: 26379 },
    { host: 'redis-sentinel-1.redis-sentinel-headless', port: 26379 },
    { host: 'redis-sentinel-2.redis-sentinel-headless', port: 26379 },
  ],
  name: 'mymaster',
  password: process.env.REDIS_PASSWORD,
  db: 0,
  retryStrategy: (times) => Math.min(times * 100, 3000),
  reconnectOnError: (err) => {
    const targetErrors = ['READONLY', 'ECONNRESET', 'ETIMEDOUT'];
    return targetErrors.some(e => err.message.includes(e));
  },
  enableReadyCheck: true,
  maxRetriesPerRequest: 3,
  connectTimeout: 5000,
});

// Cluster 模式
const cluster = new Redis.Cluster(
  [
    { host: 'redis-cluster-leader-0.redis-cluster-leader-headless', port: 6379 },
    { host: 'redis-cluster-leader-1.redis-cluster-leader-headless', port: 6379 },
    { host: 'redis-cluster-leader-2.redis-cluster-leader-headless', port: 6379 },
  ],
  {
    redisOptions: { password: process.env.REDIS_PASSWORD },
    clusterRetryStrategy: (times) => Math.min(times * 200, 5000),
    scaleReads: 'slave',  // 读从 replica
    natMap: {},  // K8s 内部无需 NAT
  }
);
```

## 性能调优

### 内核参数

```yaml
# DaemonSet 或 initContainer 设置
apiVersion: v1
kind: Pod
spec:
  initContainers:
    - name: sysctl
      image: busybox:1.36
      securityContext:
        privileged: true
      command:
        - sh
        - -c
        - |
          sysctl -w net.core.somaxconn=65535
          sysctl -w net.ipv4.tcp_max_syn_backlog=65535
          echo 1 > /proc/sys/vm/overcommit_memory
          echo never > /sys/kernel/mm/transparent_hugepage/enabled
```

### Redis 配置最佳实践

```conf
# 内存管理
maxmemory 3gb
maxmemory-policy allkeys-lru
maxmemory-samples 10

# 持久化（RDB + AOF 混合）
save 900 1
save 300 10
save 60 10000
appendonly yes
appendfsync everysec
aof-use-rdb-preamble yes

# 网络
tcp-keepalive 300
tcp-backlog 65535
timeout 0

# 性能
hz 10
dynamic-hz yes
lazyfree-lazy-eviction yes
lazyfree-lazy-expire yes
lazyfree-lazy-server-del yes
replica-lazy-flush yes

# 慢查询日志
slowlog-log-slower-than 10000
slowlog-max-len 128
```

## 监控与告警

### Prometheus 告警规则

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
        - alert: RedisDown
          expr: redis_up == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Redis 实例 {{ $labels.instance }} 宕机"
        - alert: RedisMemoryHigh
          expr: redis_memory_used_bytes / redis_memory_max_bytes > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Redis 内存使用率 > 90%"
        - alert: RedisRejectedConnections
          expr: increase(redis_rejected_connections_total[5m]) > 0
          labels:
            severity: warning
          annotations:
            summary: "Redis 拒绝连接"
        - alert: RedisReplicationBroken
          expr: redis_connected_slaves < 2
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Redis 副本数不足"
        - alert: RedisSlowLog
          expr: increase(redis_slowlog_length[5m]) > 10
          labels:
            severity: warning
          annotations:
            summary: "Redis 慢查询增多"
```

## 故障排查

| 症状 | 原因 | 排查 |
|------|------|------|
| Pod CrashLoopBackOff | OOM/配置错误 | `kubectl logs` + 检查 maxmemory |
| 连接超时 | 网络策略/资源不足 | 检查 NetworkPolicy + Pod 资源 |
| 主从切换频繁 | 网络抖动/超时太短 | 调整 down-after-milliseconds |
| 内存持续增长 | 未设置 maxmemory | 配置淘汰策略 |
| 持久化失败 | 磁盘空间/权限 | 检查 PVC 容量和 fsGroup |
| Cluster 槽位不均 | 节点容量差异 | `redis-cli --cluster rebalance` |

```bash
# 常用排查命令
kubectl exec -it redis-cluster-leader-0 -n production -- redis-cli info memory
kubectl exec -it redis-cluster-leader-0 -n production -- redis-cli info replication
kubectl exec -it redis-cluster-leader-0 -n production -- redis-cli cluster info
kubectl exec -it redis-cluster-leader-0 -n production -- redis-cli slowlog get 10
kubectl exec -it redis-cluster-leader-0 -n production -- redis-cli --bigkeys
```

## 备份与恢复

```yaml
# CronJob 定期 RDB 备份到 S3
apiVersion: batch/v1
kind: CronJob
metadata:
  name: redis-backup
  namespace: production
spec:
  schedule: "0 2 * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: backup
              image: redis:7.2-alpine
              command:
                - sh
                - -c
                - |
                  redis-cli -h redis-cluster-leader-0.redis-cluster-leader-headless BGSAVE
                  sleep 30
                  redis-cli -h redis-cluster-leader-0.redis-cluster-leader-headless --rdb /tmp/dump.rdb
                  aws s3 cp /tmp/dump.rdb s3://redis-backups/$(date +%Y%m%d)/dump.rdb
          restartPolicy: OnFailure
```

## Related

- [[07-数据库中间件/02-缓存/index.md|缓存]]
- [[07-数据库中间件/02-缓存/01-redis-cluster-sentinel-topology.md|Redis 集群拓扑]]
- [[09-可观测性/02-指标/index.md|监控指标]]
