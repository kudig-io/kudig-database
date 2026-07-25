---
title: PostgreSQL on Kubernetes 生产指南
description: 面向在 Kubernetes 上运行 PostgreSQL 的生产指南，覆盖高可用架构、Patroni/CloudNativePG 选型、备份与 PITR、连接池、监控告警与故障转移。
summary: 面向 Kubernetes 上 PostgreSQL 的生产指南，覆盖 HA、Patroni/CloudNativePG、备份/PITR、连接池与故障转移。
category: database-middleware
tags:
- production
- best-practices
- playbook
- database-middleware
- postgresql
- patroni
- cloudnativepg
- high-availability
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 数据库工程师
estimated_read_time: 30min
intent_queries:
- Kubernetes 上如何生产化运行 PostgreSQL
- Patroni 与 CloudNativePG 怎么选
- PostgreSQL on K8s 备份与 PITR 实践
- PostgreSQL 连接池与监控配置
trigger_keywords:
- PostgreSQL
- Patroni
- CloudNativePG
- PITR
- PgBouncer
- WAL-G
prerequisites:
- kubectl-basics
- postgresql-basics
- kubernetes-storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# PostgreSQL on Kubernetes 生产指南

本指南面向需要在 Kubernetes 上以生产标准运行 PostgreSQL 的 SRE 与数据库工程师，提供高可用架构、Operator 选型、备份/时间点恢复、连接池、监控与故障转移的完整操作路径。数据库是有状态工作负载中最关键的一类，其可靠性、性能与可恢复性直接影响业务连续性。在 Kubernetes 上运行 PostgreSQL 时，必须充分考虑存储性能、网络稳定性、故障检测与自动恢复机制。与无状态应用不同，数据库的运维需要理解存储、网络、复制拓扑与 Operator 行为之间的复杂关系。本指南中的命令与配置可直接在已安装 `kubectl` 与 `helm` 的环境中执行，所有重大变更应先在测试集群验证，并遵循 [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产就绪运维框架]] 中的变更管理要求。

## 1. 适用场景与范围

本指南适用于以下场景：

- 在 Kubernetes 上部署生产级 PostgreSQL 集群。
- 需要实现自动故障转移、读写分离、备份与 PITR。
- 需要配置连接池、监控告警与性能基线。
- 排查 PostgreSQL Pod 异常、复制延迟、备份失败、连接数耗尽等问题。
- 需要理解 CloudNativePG 与 Patroni 两种主流方案的差异与适用场景。

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必需工具
kubectl version
helm version

# 推荐 Operator 与工具
# CloudNativePG: https://cloudnative-pg.io/
# Patroni + Spilo/Zalando: https://github.com/zalando/spilo
# WAL-G / pgBackRest: 备份恢复
# PgBouncer: 连接池
# postgres_exporter: Prometheus 监控
```
建议为数据库节点池配置：
- 高 IOPS SSD 或云厂商等价物（如 GCP Hyperdisk、AWS io2、Azure Premium SSD v2）。
- 充足的 CPU/内存，避免与突发工作负载共享节点。
- 反亲和性，确保主从副本分布在不同节点与可用区。
- 专用网络策略，限制仅允许应用与监控组件访问数据库端口。
- 独立的磁盘用于数据与 WAL，减少 I/O 争用。

## 3. 核心概念与架构

### 3.1 高可用拓扑

典型三节点架构如下：

```
[Primary]  <--同步/异步复制-->  [Replica 1]
       |                            |
       +---------------------> [Replica 2]
```

- **Primary**：处理读写请求，是唯一可写入的节点。
- **Replica**：通过流复制接收 WAL，支持只读查询与 failover。
- **Witness / Quorum**：在 Patroni 中参与 leader 选举，不承载数据，用于降低网络分区时的脑裂风险。

生产环境建议至少部署 3 个实例，确保单点故障时仍能维持 quorum 与自动切换。对于读压力较大的场景，可以增加异步只读副本，并将只读查询路由到副本节点。

### 3.2 Operator 选型

| 方案 | 特点 | 适用场景 |
|---|---|---|
| **CloudNativePG** | 原生 K8s CRD、内置 HA/备份/PITR、活跃社区 | 新建集群、希望 K8s-native 体验 |
| **Patroni + Spilo** | 成熟、灵活、可与现有基础设施集成 | 已有 Patroni 经验、复杂定制需求 |
| **StackGres / Zalando** | 企业级功能、多租户、GUI | 大规模多团队数据库平台 |

生产建议：新集群优先使用 CloudNativePG；已有 Patroni 运维经验可继续使用 Spilo/Patroni。无论选择哪种方案，都应确保团队熟悉其故障转移行为、备份恢复流程与升级路径。

### 3.3 存储与性能

- 使用 StatefulSet + PVC，存储类选择支持快照与扩容的 CSI。
- PostgreSQL 数据目录与 WAL 目录建议分离，重要场景使用独立 PV。
- 启用 `wal_level = replica`、`max_wal_senders`、`hot_standby = on`。
- 根据工作负载调整 `shared_buffers`、`effective_cache_size`、`work_mem`、`maintenance_work_mem` 等参数。
- 监控 `pg_stat_user_tables`、`pg_stat_database`、`pg_stat_bgwriter` 等视图，识别性能瓶颈。

## 4. 标准操作流程

### 4.1 使用 CloudNativePG 部署集群

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Operator
kubectl apply --server-side -f \
  https://raw.githubusercontent.com/cloudnative-pg/cloudnative-pg/release-1.24/releases/cnpg-1.24.0.yaml

# 创建集群
cat <<EOF | kubectl apply -f -
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: prod-pg
  namespace: database
spec:
  instances: 3
  imageName: ghcr.io/cloudnative-pg/postgresql:16.4
  storage:
    size: 100Gi
    storageClass: premium-rwo
  postgresql:
    parameters:
      max_connections: "200"
      shared_buffers: "2GB"
      effective_cache_size: "6GB"
  affinity:
    enablePodAntiAffinity: true
    topologyKey: topology.kubernetes.io/zone
  backup:
    retentionPolicy: "30d"
    barmanObjectStore:
      destinationPath: s3://prod-pg-backups/
      s3Credentials:
        accessKeyId:
          name: pg-backup-secret
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: pg-backup-secret
          key: SECRET_ACCESS_KEY
EOF
```
创建后应验证 Pod 状态、主从复制关系以及 Service 端点是否正常工作。

### 4.2 使用 Patroni 部署（Helm 示例）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Zalando Postgres Operator
helm repo add postgres-operator-charts \
  https://opensource.zalando.com/postgres-operator/charts/postgres-operator
helm install postgres-operator postgres-operator-charts/postgres-operator -n database --create-namespace

# 创建集群
cat <<EOF | kubectl apply -f -
apiVersion: acid.zalan.do/v1
kind: postgresql
metadata:
  name: prod-pg
  namespace: database
spec:
  teamId: prod
  volume:
    size: 100Gi
    storageClass: premium-rwo
  numberOfInstances: 3
  users:
    app:
    - superuser
    - createdb
  databases:
    appdb: app
  postgresql:
    version: "16"
  resources:
    requests:
      cpu: "2"
      memory: 4Gi
    limits:
      cpu: "4"
      memory: 8Gi
EOF
```
### 4.3 备份与 PITR

CloudNativePG 示例：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 立即执行基础备份
kubectl cnpg backup prod-pg -n database

# 查看备份
kubectl get backups -n database

# 按时间点恢复（PITR）
cat <<EOF | kubectl apply -f -
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: prod-pg-restore
  namespace: database
spec:
  instances: 3
  storage:
    size: 100Gi
    storageClass: premium-rwo
  bootstrap:
    recovery:
      source: prod-pg
      recoveryTarget:
        targetTime: "2026-07-01T10:00:00Z"
EOF
```
Patroni + WAL-G 示例：

```bash
# 配置 WAL-G 环境变量
WALE_S3_PREFIX=s3://prod-pg-backups/wal
AWS_ACCESS_KEY_ID=xxx
AWS_SECRET_ACCESS_KEY=xxx

# 执行备份
wal-g backup-push /var/lib/postgresql/data

# 按时间点恢复
wal-g backup-fetch /var/lib/postgresql/data LATEST
# 创建 recovery.signal 并配置 restore_command
```

备份策略应包括：每日全量备份、持续 WAL 归档、定期恢复演练。PITR 能力取决于 WAL 归档的完整性与保留期，建议保留期至少覆盖两个全量备份周期。

### 4.4 连接池（PgBouncer）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Helm 部署 PgBouncer
helm repo add pgbouncer https://cowboysysop.github.io/charts/
helm install pgbouncer cowboysysop/pgbouncer -n database \
  --set config.databases.prod=host=prod-pg-rw.database.svc.cluster.local port=5432 dbname=appdb
```
关键参数：
- `max_client_conn`：根据应用连接数设置，建议 ≥ 10000。
- `default_pool_size`：一般设置为 `max_connections / 实例数`。
- `pool_mode`：事务池（transaction）适合短连接Web应用；会话池（session）适合长连接分析任务。

生产建议：所有应用应通过 PgBouncer 访问数据库，避免直接占用大量 PostgreSQL 后端连接。长事务或分析任务应使用独立的连接池或直连只读副本。

### 4.5 监控与告警

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 部署 postgres_exporter
helm install pg-exporter prometheus-community/prometheus-postgres-exporter \
  -n monitoring \
  --set config.datasource.host=prod-pg-rw.database.svc.cluster.local \
  --set config.datasource.user=exporter \
  --set config.datasource.password=<PASSWORD>
```
关键指标：
- `pg_up`：实例存活。
- `pg_stat_activity_count` / `pg_settings_max_connections`：连接数使用率。
- `pg_stat_replication_lag`：复制延迟。
- `pg_stat_database_deadlocks`：死锁数量。
- `pg_stat_database_blks_hit` / `pg_stat_database_blks_read`：缓冲命中率。

### 4.6 故障转移

CloudNativePG 自动执行 failover。手动触发：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前主节点
kubectl get cluster prod-pg -n database -o jsonpath='{.status.currentPrimary}'

# 手动切换主节点（滚动升级或维护场景）
kubectl cnpg failover prod-pg -n database <target-pod>

# 查看复制状态
kubectl cnpg status prod-pg -n database
```
Patroni 手动切换：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入 Patroni leader Pod
kubectl exec -it prod-pg-0 -n database -- patronictl list
kubectl exec -it prod-pg-0 -n database -- patronictl switchover
```
## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| 集群状态 | `kubectl get cluster -n database` / `patronictl list` | 1 primary + N replica，状态正常 |
| 复制延迟 | `kubectl cnpg status prod-pg -n database` | 延迟 < 1s |
| 连接数 | `psql -c "SELECT count(*) FROM pg_stat_activity;"` | 连接数 < max_connections 的 80% |
| 备份状态 | `kubectl get backups -n database` | 最近备份 Completed |
| WAL 归档 | `psql -c "SELECT archived_count, failed_count FROM pg_stat_archiver;"` | failed_count = 0 |
| 慢查询 | `psql -c "SELECT * FROM pg_stat_statements ORDER BY total_exec_time DESC LIMIT 10;"` | 无异常高耗时查询 |

## 6. 常见故障与 remediation

| 现象 | 根因 | 处理命令/步骤 |
|---|---|---|
| Pod 持续 CrashLoopBackOff | 数据目录损坏、配置错误、资源不足 | 查看 Pod 日志；检查 PVC 挂载与资源请求 |
| 复制延迟高 | 网络抖动、大事务、I/O 瓶颈 | 检查 `pg_stat_replication`；优化大事务或扩容 I/O |
| 主节点无法选举 | Etcd/DCS 不可用、网络分区 | 检查 Patroni DCS 健康；确认 Pod 间网络连通 |
| 备份失败 | 对象存储凭证过期、网络不可达 | 验证 Secret；检查备份 Pod 日志 |
| 连接数耗尽 | 应用未使用连接池、连接泄漏 | 部署 PgBouncer；检查应用连接生命周期 |
| 慢查询导致性能下降 | 缺少索引、统计信息过期 | 启用 `pg_stat_statements`；执行 `ANALYZE` 与索引优化 |
| PVC 容量不足 | 数据增长未扩容 | 调整 PVC size 或执行 `kubectl edit pvc`（需存储类支持扩容） |

## 7. 风险与注意事项

1. **存储性能是 PostgreSQL 瓶颈**：生产环境必须使用高 IOPS 存储，避免使用普通 HDD 或共享节点 IO。
2. **备份必须验证可恢复**：每月执行 PITR 演练，确认 RPO/RTO 满足业务要求。
3. **故障转移可能导致短暂写入中断**：应用需具备连接重试与事务幂等能力。
4. **max_connections 与 PgBouncer 配合**：直接连接数不宜过高，长事务避免使用 transaction pool mode。
5. **升级前必须验证大版本兼容性**：PostgreSQL 大版本升级需要逻辑复制或 pg_upgrade，不能原地滚动。
6. **Secret 与凭据轮换**：定期轮换数据库用户密码与对象存储访问密钥，使用 External Secrets Operator 自动化。
7. **网络隔离**：通过 NetworkPolicy 限制仅允许应用访问数据库端口，避免暴露在集群外部。
8. **资源竞争**：数据库 Pod 应配置 Guaranteed QoS，避免与突发工作负载共享节点。

## 8. 相关 Runbook / 推荐阅读

- [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[07-数据库中间件/01-数据库/02-postgresql-enterprise-database.md|PostgreSQL 企业数据库]]
- [[07-数据库中间件/01-数据库/99-cloudnativepg-enterprise-guide.md|CloudNativePG 企业指南]]
- [[06-存储/00-总览/99-production-readiness-operations-guide.md|存储数据域生产就绪指南]]
- [[12-可靠性/README.md|可靠性工程域]]
- [[08-安全/README.md|安全合规域]]


<!-- risk-assessed -->
