---
title: Database Operator Production Operations — CloudNativePG, MySQL Operator, and Day-2 Operations
description: K8s 数据库 Operator 生产运维 — CloudNativePG 集群管理、MySQL InnoDB Cluster、备份恢复、故障转移、性能调优
summary: 数据库 Operator 的生产级运维实践，涵盖集群管理、故障转移、备份策略与性能优化
category: practice
tags:
- database-operator
- cloudnativepg
- mysql-operator
- day2-operations
- high-availability
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: database
---
# 数据库 Operator 生产运维实践

> CloudNativePG、MySQL Operator 等数据库 Operator 的生产级 Day-2 运维。

## Operator 选型对比

| Operator | 数据库 | HA 方案 | 备份 | 连接池 | 成熟度 |
|----------|--------|---------|------|--------|--------|
| CloudNativePG | PostgreSQL | 流复制 + 自动 Failover | 对象存储 | PgBouncer 内置 | 生产就绪 |
| MySQL Operator (Oracle) | MySQL | InnoDB Cluster (Group Replication) | 对象存储 | MySQL Router | 生产就绪 |
| Percona Operator | MySQL/PG/MongoDB | 各引擎原生 HA | 对象存储 | ProxySQL/HAProxy | 生产就绪 |
| Zalando Postgres Operator | PostgreSQL | Patroni + Spilo | WAL-G | PgBouncer | 生产就绪 |
| KubeBlocks | 多引擎 | 各引擎原生 | 对象存储 | 内置 | 快速成长 |

## CloudNativePG 生产部署

### 集群定义

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: postgres-prod
  namespace: database
spec:
  instances: 3
  imageName: ghcr.io/cloudnative-pg/postgresql:16.3
  
  storage:
    size: 100Gi
    storageClass: gp3-encrypted
  
  walStorage:
    size: 20Gi
    storageClass: gp3-encrypted
  
  postgresql:
    parameters:
      shared_buffers: "2GB"
      effective_cache_size: "6GB"
      work_mem: "64MB"
      maintenance_work_mem: "512MB"
      max_connections: "200"
      max_wal_size: "4GB"
      checkpoint_completion_target: "0.9"
      random_page_cost: "1.1"
      effective_io_concurrency: "200"
      max_worker_processes: "8"
      max_parallel_workers: "4"
      max_parallel_workers_per_gather: "2"
      log_min_duration_statement: "1000"
      log_checkpoints: "on"
      log_lock_waits: "on"
      log_temp_files: "0"
      track_activity_query_size: "4096"
    pg_hba:
      - host all all 10.0.0.0/8 scram-sha-256
  
  resources:
    requests:
      cpu: "2"
      memory: 8Gi
    limits:
      cpu: "4"
      memory: 12Gi
  
  affinity:
    topologyKey: topology.kubernetes.io/zone
  
  bootstrap:
    initdb:
      database: app
      owner: app_user
      secret:
        name: postgres-credentials
  
  backup:
    barmanObjectStore:
      destinationPath: s3://pg-backups/production/
      s3Credentials:
        accessKeyId:
          name: backup-creds
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: backup-creds
          key: SECRET_ACCESS_KEY
      wal:
        compression: gzip
        maxParallel: 4
      data:
        compression: gzip
    retentionPolicy: "30d"
  
  monitoring:
    enablePodMonitor: true
    customQueriesConfigMap:
      - name: cnpg-default-monitoring
        key: custom-queries
  
  nodeMaintenanceWindow:
    inProgress: false
    reusePVC: true
```

### 定时备份

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: ScheduledBackup
metadata:
  name: postgres-prod-daily
  namespace: database
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点
  backupOwnerReference: self
  cluster:
    name: postgres-prod
  immediate: true  # 创建时立即执行一次
```

### 连接池（PgBouncer）

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Pooler
metadata:
  name: postgres-prod-pooler
  namespace: database
spec:
  cluster:
    name: postgres-prod
  instances: 3
  type: rw  # rw 或 ro
  pgbouncer:
    poolMode: transaction
    parameters:
      max_client_conn: "1000"
      default_pool_size: "25"
      min_pool_size: "5"
      reserve_pool_size: "5"
      reserve_pool_timeout: "3"
      server_idle_timeout: "300"
      server_lifetime: "3600"
  monitoring:
    enablePodMonitor: true
```

## MySQL Operator（Oracle）

### InnoDB Cluster

```yaml
apiVersion: mysql.oracle.com/v2
kind: InnoDBCluster
metadata:
  name: mysql-prod
  namespace: database
spec:
  secretName: mysql-root-secret
  instances: 3
  tlsUseSelfSigned: true
  
  router:
    instances: 2
    routingOptions:
      readWrite:
        maxConnections: 512
      readOnly:
        maxConnections: 1024
  
  datadirVolumeClaimTemplate:
    accessModes: ["ReadWriteOnce"]
    resources:
      requests:
        storage: 100Gi
    storageClassName: gp3-encrypted
  
  mycnf: |
    [mysqld]
    innodb_buffer_pool_size = 4G
    innodb_log_file_size = 1G
    innodb_flush_log_at_trx_commit = 1
    innodb_flush_method = O_DIRECT
    max_connections = 500
    slow_query_log = 1
    long_query_time = 1
    binlog_expire_logs_seconds = 604800
  
  backupProfiles:
    - name: daily-backup
      dumpInstance:
        dumpOptions:
          databases: ["app"]
        storage:
          s3:
            bucketName: mysql-backups
            prefix: production
            config: s3-credentials
            endpoint: s3.amazonaws.com
  
  backupSchedules:
    - name: daily
      schedule: "0 3 * * *"
      backupProfileName: daily-backup
      enabled: true
  
  podSpec:
    containers:
      - name: mysql
        resources:
          requests:
            cpu: "2"
            memory: 6Gi
          limits:
            cpu: "4"
            memory: 8Gi
```

## Day-2 运维操作

### 故障转移验证

```bash
# CloudNativePG: 查看集群状态
kubectl cnpg status postgres-prod -n database
# 输出: 主节点、副本同步状态、WAL 位点

# 模拟主节点故障
kubectl delete pod postgres-prod-1 -n database --grace-period=0
# 观察自动 Failover（通常 < 30s）
kubectl cnpg status postgres-prod -n database -w

# 手动 Switchover（计划维护）
kubectl cnpg promote postgres-prod -n database --target postgres-prod-2
```

### 在线扩容

```bash
# 增加副本数（3 → 5）
kubectl patch cluster postgres-prod -n database \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/instances","value":5}]'

# 扩容存储（需 StorageClass 支持）
kubectl patch cluster postgres-prod -n database \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/storage/size","value":"200Gi"}]'
```

### 从备份恢复

```yaml
# 从对象存储恢复到新集群
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: postgres-recovered
  namespace: database
spec:
  instances: 3
  storage:
    size: 100Gi
  bootstrap:
    recovery:
      source: production-backup
      recoveryTarget:
        targetTime: "2026-07-20 14:30:00+08"  # PITR
  externalClusters:
    - name: production-backup
      barmanObjectStore:
        destinationPath: s3://pg-backups/production/
        s3Credentials:
          accessKeyId:
            name: backup-creds
            key: ACCESS_KEY_ID
          secretAccessKey:
            name: backup-creds
            key: SECRET_ACCESS_KEY
        wal:
          maxParallel: 4
```

## 监控告警

### 关键指标

```promql
# CloudNativePG
cnpg_pg_replication_in_recovery{namespace="database"}  # 是否恢复中
cnpg_pg_replication_is_wal_receiver_up{namespace="database"}  # WAL 接收
cnpg_collector_pg_wal_archive_status{value="ready"}  # WAL 归档积压
cnpg_backends_total{namespace="database"}  # 连接数
cnpg_pg_database_size_bytes{namespace="database"}  # 数据库大小

# 告警规则
- alert: PostgresReplicationLag
  expr: cnpg_pg_replication_lag > 30
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "PostgreSQL 复制延迟 > 30s"

- alert: PostgresBackupFailed
  expr: cnpg_collector_last_failed_backup_timestamp > cnpg_collector_last_available_backup_timestamp
  labels:
    severity: critical
  annotations:
    summary: "PostgreSQL 备份失败"
```

## 故障排查

| 症状 | 原因 | 排查 |
|------|------|------|
| Pod CrashLoopBackOff | 配置错误/磁盘满 | `kubectl logs` + `kubectl describe` |
| 复制延迟增大 | 网络/大事务/WAL 积压 | `cnpg status` + 检查 WAL |
| Failover 失败 | Quorum 不足（2/3 节点） | 检查 Pod 分布 |
| 备份失败 | S3 权限/空间不足 | 检查 backup 日志 |
| 连接池耗尽 | max_connections 过低 | 调整 PgBouncer 参数 |
| 性能下降 | 参数不当/锁等待 | `pg_stat_activity` + 慢查询日志 |

```bash
# 常用诊断命令
kubectl cnpg status postgres-prod -n database
kubectl cnpg report postgres-prod -n database  # 完整报告
kubectl exec -it postgres-prod-1 -n database -- psql -U postgres -c "SELECT * FROM pg_stat_replication;"
kubectl exec -it postgres-prod-1 -n database -- psql -U postgres -c "SELECT * FROM pg_stat_activity WHERE state != 'idle';"
kubectl exec -it postgres-prod-1 -n database -- psql -U postgres -c "SELECT * FROM pg_locks WHERE NOT granted;"
```

## Related

- [[数据库中间件/Operator管理/index.md|Operator 管理]]
- [[数据库中间件/Operator管理/02-operator-comparison-mysql-postgres-redis.md|Operator 对比]]
- [[数据库中间件/数据库/16-postgresql-kubernetes-production-guide.md|PostgreSQL 生产指南]]
