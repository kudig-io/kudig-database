# CloudNativePG 企业级 PostgreSQL 运维指南

> **适用版本**: CloudNativePG v1.25  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、架构概览](#一架构概览)
- [二、Operator 部署](#二operator-部署)
- [三、集群创建与配置](#三集群创建与配置)
- [四、高可用与故障转移](#四高可用与故障转移)
- [五、备份与恢复](#五备份与恢复)
- [六、监控与告警](#六监控与告警)
- [七、连接池与多租户](#七连接池与多租户)
- [八、升级与维护](#八升级与维护)
- [九、生态集成](#九生态集成)

---

## 一、架构概览

```
CloudNativePG 架构
├── Operator (Deployment)
│   ├── Cluster Controller
│   ├── Backup Controller
│   ├── Pooler Controller (PgBouncer)
│   └── ScheduledBackup Controller
│
├── PostgreSQL Cluster (StatefulSet)
│   ├── Primary (读写)
│   ├── Replica 1 (流复制)
│   ├── Replica 2 (流复制)
│   └── ... (可扩展)
│
├── 存储 (PVC per Pod)
│   ├── 数据卷 (PGDATA)
│   └── WAL 卷 (可选分离)
│
├── 备份 (ScheduledBackup)
│   ├── 对象存储 (S3 / GCS / Azure Blob)
│   ├── WAL 归档 (连续归档)
│   └── 时间点恢复 (PITR)
│
└── 连接池 (Pooler / PgBouncer)
    ├── 读写分离
    └── 连接复用
```

### 核心特性

| 特性 | 说明 |
|:---|:---|
| 原生 K8s 集成 | CRD 驱动，无额外依赖 |
| 流复制 HA | 异步/同步复制，自动故障转移 |
| 时间点恢复 | 基于 WAL 归档的 PITR |
| 对象存储备份 | S3/GCS/Azure Blob 兼容 |
| 内置监控 | Prometheus 指标导出 |
| 连接池 | 内置 PgBouncer 集成 |
| 滚动升级 | 零停机版本升级 |

---

## 二、Operator 部署

```bash
# 安装 Operator
kubectl apply -f \
  https://raw.githubusercontent.com/cloudnative-pg/cloudnative-pg/release-1.25/releases/cnpg-1.25.0.yaml

# 或 Helm
helm repo add cnpg https://cloudnative-pg.github.io/charts
helm install cnpg cnpg/cloudnative-pg \
  --namespace cnpg-system \
  --create-namespace
```

### 验证安装

```bash
kubectl get deployment -n cnpg-system cnpg-controller-manager
kubectl get crd | grep postgresql.cnpg.io
```

---

## 三、集群创建与配置

### 3.1 基础集群

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: production-db
  namespace: database
spec:
  instances: 3
  
  # PostgreSQL 版本
  imageName: ghcr.io/cloudnative-pg/postgresql:17.4
  
  # 存储配置
  storage:
    size: 100Gi
    storageClass: gp3
  
  # WAL 分离存储 (可选，推荐高负载场景)
  walStorage:
    size: 50Gi
    storageClass: gp3
  
  # 资源配置
  resources:
    requests:
      memory: "4Gi"
      cpu: "2"
    limits:
      memory: "8Gi"
      cpu: "4"
  
  # 优先级与亲和性
  priorityClassName: database-critical
  affinity:
    enablePodAntiAffinity: true
    topologyKey: kubernetes.io/hostname
  
  # PostgreSQL 参数
  postgresql:
    parameters:
      max_connections: "200"
      shared_buffers: "2GB"
      effective_cache_size: "6GB"
      maintenance_work_mem: "512MB"
      checkpoint_completion_target: "0.9"
      wal_buffers: "16MB"
      default_statistics_target: "100"
      random_page_cost: "1.1"
      effective_io_concurrency: "200"
      work_mem: "10485kB"
      min_wal_size: "2GB"
      max_wal_size: "8GB"
  
  # 监控
  monitoring:
    enabled: true
    customQueriesConfigMap:
      name: cnpg-custom-queries
      key: queries
  
  # 备份配置
  backup:
    enabled: true
    retentionPolicy: "30d"
    barmanObjectStore:
      destinationPath: "s3://my-backups/postgres/production-db"
      s3Credentials:
        accessKeyId:
          name: aws-creds
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: aws-creds
          key: ACCESS_SECRET_KEY
      wal:
        compression: gzip
        maxParallel: 8
```

### 3.2 初始化脚本与数据库

```yaml
spec:
  bootstrap:
    initdb:
      database: appdb
      owner: appuser
      secret:
        name: appuser-password
      postInitSQLRefs:
        configMapRefs:
        - name: init-scripts
          key: create_extensions.sql
          optional: false
```

```sql
-- create_extensions.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS uuid-ossp;
CREATE EXTENSION IF NOT EXISTS pg_stat_statements;
```

---

## 四、高可用与故障转移

### 4.1 同步复制配置

```yaml
spec:
  postgresql:
    synchronous:
      method: any
      number: 1  # 至少 1 个同步副本
      dataDurability: preferred  # required | preferred
```

### 4.2 故障转移行为

```
故障检测
    |
    ├── health check 失败 (默认 30s 超时)
    |
    ▼
选举新主 (基于 LSN 最新者)
    |
    ▼
 promote 新主
    |
    ▼
更新 Service Endpoint
    |
    ▼
应用重连 (需 connection pooler)
```

### 4.3 手动切换

```bash
# 查看集群状态
kubectl cnpg status production-db -n database

# 手动切换主节点
kubectl cnpg promote production-db <pod-name> -n database

# 查看复制延迟
kubectl cnpg replication production-db -n database
```

---

## 五、备份与恢复

### 5.1 定时备份

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: ScheduledBackup
metadata:
  name: production-daily-backup
  namespace: database
spec:
  schedule: "0 2 * * *"
  backupOwnerReference: self
  cluster:
    name: production-db
  method: barmanObjectStore
```

### 5.2 按需备份

```bash
# 立即执行备份
kubectl apply -f - <<EOF
apiVersion: postgresql.cnpg.io/v1
kind: Backup
metadata:
  name: manual-backup
  namespace: database
spec:
  cluster:
    name: production-db
  method: barmanObjectStore
EOF

# 查看备份状态
kubectl get backups -n database
```

### 5.3 时间点恢复 (PITR)

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: production-db-recovery
  namespace: database
spec:
  instances: 3
  storage:
    size: 100Gi
  
  bootstrap:
    recovery:
      source: production-db
      recoveryTarget:
        targetTime: "2026-04-24 15:30:00+00"
```

### 5.4 对象存储端点兼容性

| 提供商 | destinationPath | 额外配置 |
|:---|:---|:---|
| AWS S3 | s3://bucket/path | s3Credentials |
| GCS | gs://bucket/path | googleCredentials |
| Azure Blob | https://account.blob.core.windows.net/container | azureCredentials |
| MinIO | s3://bucket/path | endpointURL, pathStyle |

---

## 六、监控与告警

### 6.1 Prometheus ServiceMonitor

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PodMonitor
metadata:
  name: cnpg-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      cnpg.io/cluster: production-db
  namespaceSelector:
    matchNames:
      - database
  podMetricsEndpoints:
  - port: metrics
    interval: 30s
```

### 6.2 关键告警规则

```yaml
- alert: CNPGClusterNotHealthy
  expr: cnpg_collector_up == 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "CloudNativePG 集群不健康"

- alert: CNPGReplicationLag
  expr: cnpg_pg_stat_replication_pg_wal_lsn_diff / 1024 / 1024 > 100
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "PostgreSQL 复制延迟超过 100MB"

- alert: CNPGBackupFailed
  expr: cnpg_backups_count{phase!="completed"} > 0
  for: 1h
  labels:
    severity: critical
  annotations:
    summary: "CloudNativePG 备份失败"

- alert: CNPGConnectionsHigh
  expr: |
    cnpg_pg_stat_activity_count / cnpg_pg_settings_setting{name="max_connections"} > 0.8
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "PostgreSQL 连接数接近上限"
```

---

## 七、连接池与多租户

### 7.1 Pooler (PgBouncer) 连接池

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Pooler
metadata:
  name: production-db-pooler
  namespace: database
spec:
  cluster:
    name: production-db
  instances: 2
  type: rw           # rw (读写) | ro (只读)
  pgbouncer:
    poolMode: transaction
    parameters:
      max_client_conn: "10000"
      default_pool_size: "25"
      reserve_pool_size: "5"
      reserve_pool_timeout: "3"
      server_idle_timeout: "600"
      server_lifetime: "3600"
      query_timeout: "0"
      query_wait_timeout: "120"
```

### 7.2 命名空间隔离

```yaml
# team-a-database.yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: team-a-db
  namespace: team-a
spec:
  instances: 2
  storage:
    size: 50Gi
  # 独立的备份路径
  backup:
    barmanObjectStore:
      destinationPath: "s3://my-backups/postgres/team-a"
```

---

## 八、升级与维护

### 8.1 滚动升级

```bash
# 修改 imageName 后应用，Operator 自动执行滚动升级
kubectl edit cluster production-db -n database
# spec.imageName: ghcr.io/cloudnative-pg/postgresql:17.5

# 监控升级进度
kubectl cnpg status production-db -n database
```

### 8.2 升级顺序

```
1. 先升级 Replica (逐一重启)
2. 执行 switchover (将 Replica 提升为主)
3. 升级旧主 (现为 Replica)
4. 可选: 切回原始主节点
```

### 8.3 维护窗口

```yaml
spec:
  nodeMaintenanceWindow:
    enabled: true
    inProgress: true  # 标记维护中，阻止自动故障转移
```

---

## 九、生态集成

### 典型数据管道

```
CloudNativePG (PostgreSQL)
    |
    ├── Debezium CDC ---> Kafka (Strimzi)
    |                        |
    |                        ├── Consumer: 数据仓库
    |                        ├── Consumer: 缓存更新
    |                        └── Consumer: 事件驱动微服务
    |
    └── 定期备份 ---> S3 (长期归档)
```

### 与生态工具对比

| 工具 | 类型 | 与 CloudNativePG 关系 |
|:---|:---|:---|
| Strimzi | Kafka Operator | CDC 目标消息队列 |
| Debezium | CDC 工具 | PostgreSQL 变更捕获 |
| PgBouncer | 连接池 | CloudNativePG 内置集成 |
| Barman | 备份工具 | CloudNativePG 底层使用 |
| pgAdmin | 管理工具 | 独立管理 UI |

---

## 参考链接

- [CloudNativePG 官方文档](https://cloudnative-pg.io/documentation/)
- [CloudNativePG GitHub](https://github.com/cloudnative-pg/cloudnative-pg)
- [PostgreSQL 17 文档](https://www.postgresql.org/docs/17/)
- [Barman 备份文档](https://docs.pgbarman.org/)
- [Strimzi Kafka Operator](https://strimzi.io/)
- [Debezium CDC](https://debezium.io/)
