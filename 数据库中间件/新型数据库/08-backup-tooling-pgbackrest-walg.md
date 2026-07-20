---
title: "数据库备份工具链（pgBackRest/WAL-G/XtraBackup）"
description: "覆盖 PostgreSQL 和 MySQL 备份工具在 K8s 中的 PITR 配置、自动化验证与跨集群存储"
summary: "PostgreSQL 备份（pgBackRest/WAL-G/barman），MySQL 备份（Percona XtraBackup/mysqldump），K8s CronJob 备份，PITR 配置，备份验证自动化，跨集群备份存储，备份失败与恢复不一致排查"
category: 数据库中间件
tags:
- database
- backup
- pgbackrest
- wal-g
- xtrabackup
- pitr
- disaster-recovery
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
- "PostgreSQL 备份如何实现 PITR"
- "pgBackRest 在 K8s 上如何配置"
- "数据库备份验证自动化"
trigger_keywords:
- 数据库备份
- pgBackRest
- WAL-G
- XtraBackup
- PITR
- 恢复
- 备份验证
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

# 数据库备份工具链（pgBackRest/WAL-G/XtraBackup）

## 概述

数据库备份是灾难恢复的最后一道防线。在 Kubernetes 环境中，Pod 的短暂性（ephemeral）使得数据持久化和备份策略更加关键。一个完善的备份体系需要覆盖：全量备份、增量备份、WAL 归档（实现 PITR）、备份验证和异地存储。

本文覆盖 PostgreSQL（pgBackRest、WAL-G、barman）和 MySQL（Percona XtraBackup）的备份工具链在 K8s 中的生产实践，是 [[可靠性/备份恢复/]] 在数据库层的具体实现。

## 架构与核心概念

### 备份工具对比

| 工具 | 数据库 | 备份类型 | PITR | 并行压缩 | 增量备份 | K8s 集成 |
|------|--------|---------|------|---------|---------|---------|
| **pgBackRest** | PostgreSQL | 全量/差异/增量 | 支持 | 支持（多进程） | 支持 | Operator 内置 |
| **WAL-G** | PostgreSQL | 全量 + WAL | 支持 | 支持（LZ4/ZSTD） | 增量（delta） | Sidecar/CronJob |
| **barman** | PostgreSQL | 全量 + WAL | 支持 | 有限 | 不支持 | 独立 Pod |
| **XtraBackup** | MySQL/InnoDB | 全量/增量 | 支持（binlog） | 支持 | 支持 | CronJob |
| **mysqldump** | MySQL | 逻辑备份 | 需 binlog | 不支持 | 不支持 | CronJob |
| **mysqlpump** | MySQL | 逻辑备份（并行） | 需 binlog | 有限 | 不支持 | CronJob |

### PITR（Point-in-Time Recovery）原理

```
时间线：
  全量备份 (Base Backup)          恢复目标点
       |                              |
  ─────┼──────────────────────────────┼──────────→ 时间
       |    WAL 归档（连续日志流）     |
       └──────────────────────────────┘
       
恢复过程：
  1. 恢复最近的全量备份
  2. 按顺序重放 WAL 日志
  3. 到达目标时间点后停止
  4. 数据库进入一致状态
```

### 备份策略设计

| 策略 | 频率 | 保留期 | RPO | 存储开销 |
|------|------|--------|-----|---------|
| 全量备份 | 每日/每周 | 7-30 天 | 24h | 高 |
| 差异备份 | 每日 | 7 天 | 24h | 中 |
| 增量备份 | 每 6h | 3 天 | 6h | 低 |
| WAL 归档 | 连续 | 7 天 | < 5min | 取决于写入量 |
| 逻辑备份 | 每周 | 90 天 | 7 天 | 高（文本格式） |

## 生产部署

### pgBackRest 配置（CloudNativePG Operator）

```yaml
# 🟡 中风险：CloudNativePG 集群配置（含 pgBackRest 备份到 S3）
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: postgres-production
  namespace: database
spec:
  instances: 3
  storage:
    storageClass: gp3-encrypted
    size: 200Gi
  postgresql:
    parameters:
      archive_timeout: "300"
      max_wal_senders: "10"
      wal_level: "replica"
  backup:
    barmanObjectStore:
      destinationPath: "s3://db-backups/production/postgres/"
      endpointURL: "https://s3.amazonaws.com"
      s3Credentials:
        accessKeyId:
          name: backup-s3-creds
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: backup-s3-creds
          key: SECRET_ACCESS_KEY
      wal:
        compression: zstd
        maxParallel: 4
      data:
        compression: zstd
        jobs: 4
    retentionPolicy: "30d"
  resources:
    requests:
      cpu: "4"
      memory: 8Gi
    limits:
      cpu: "8"
      memory: 16Gi
---
# 定时备份计划
apiVersion: postgresql.cnpg.io/v1
kind: ScheduledBackup
metadata:
  name: postgres-daily-backup
  namespace: database
spec:
  schedule: "0 2 * * *"
  backupOwnerReference: self
  cluster:
    name: postgres-production
  immediate: true
```

### WAL-G 独立部署（CronJob）

```yaml
# 🟡 中风险：WAL-G 备份 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: walg-backup
  namespace: database
spec:
  schedule: "0 3 * * *"
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      backoffLimit: 2
      activeDeadlineSeconds: 7200
      template:
        spec:
          restartPolicy: Never
          containers:
          - name: walg
            image: wal-g/wal-g:3.0.1
            command:
            - /bin/bash
            - -c
            - |
              set -e
              echo "Starting WAL-G base backup..."
              wal-g backup-push /var/lib/postgresql/data
              echo "Backup completed. Verifying..."
              wal-g backup-list
              echo "Deleting old backups (retain 7)..."
              wal-g delete retain 7 --confirm
            env:
            - name: WALG_S3_PREFIX
              value: "s3://db-backups/production/walg"
            - name: AWS_ACCESS_KEY_ID
              valueFrom:
                secretKeyRef:
                  name: backup-s3-creds
                  key: ACCESS_KEY_ID
            - name: AWS_SECRET_ACCESS_KEY
              valueFrom:
                secretKeyRef:
                  name: backup-s3-creds
                  key: SECRET_ACCESS_KEY
            - name: WALG_COMPRESSION_METHOD
              value: "zstd"
            - name: WALG_DISK_RATE_LIMIT
              value: "104857600"
            - name: WALG_NETWORK_RATE_LIMIT
              value: "104857600"
            - name: PGHOST
              value: "postgres-primary.database.svc"
            - name: PGUSER
              value: "backup_user"
            - name: PGPASSWORD
              valueFrom:
                secretKeyRef:
                  name: backup-db-creds
                  key: password
            resources:
              requests:
                cpu: "2"
                memory: 2Gi
              limits:
                cpu: "4"
                memory: 4Gi
            volumeMounts:
            - name: pg-data
              mountPath: /var/lib/postgresql/data
              readOnly: true
          volumes:
          - name: pg-data
            persistentVolumeClaim:
              claimName: postgres-primary-data
```

### Percona XtraBackup（MySQL）

```yaml
# 🔴 高风险：XtraBackup 全量备份 CronJob（MySQL）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: xtrabackup-full
  namespace: database
spec:
  schedule: "0 1 * * 0"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      backoffLimit: 1
      activeDeadlineSeconds: 14400
      template:
        spec:
          restartPolicy: Never
          containers:
          - name: xtrabackup
            image: percona/percona-xtrabackup:8.4
            command:
            - /bin/bash
            - -c
            - |
              set -e
              BACKUP_DIR="/backups/full-$(date +%Y%m%d-%H%M%S)"
              mkdir -p "${BACKUP_DIR}"

              echo "Starting XtraBackup full backup..."
              xtrabackup --backup \
                --host=mysql-primary.database.svc \
                --port=3306 \
                --user="${BACKUP_USER}" \
                --password="${BACKUP_PASSWORD}" \
                --target-dir="${BACKUP_DIR}" \
                --parallel=4 \
                --compress=zstd \
                --compress-threads=4 \
                --slave-info \
                --safe-slave-backup

              echo "Preparing backup..."
              xtrabackup --prepare --target-dir="${BACKUP_DIR}"

              echo "Uploading to S3..."
              aws s3 cp "${BACKUP_DIR}" "s3://db-backups/production/mysql/${BACKUP_DIR}/" --recursive

              echo "Backup completed successfully"
            env:
            - name: BACKUP_USER
              valueFrom:
                secretKeyRef:
                  name: backup-db-creds
                  key: username
            - name: BACKUP_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: backup-db-creds
                  key: password
            resources:
              requests:
                cpu: "2"
                memory: 4Gi
              limits:
                cpu: "4"
                memory: 8Gi
            volumeMounts:
            - name: backup-staging
              mountPath: /backups
          volumes:
          - name: backup-staging
            emptyDir:
              sizeLimit: 100Gi
```

## 运维操作

### PITR 恢复

```bash
# 🔴 高风险：使用 CloudNativePG 执行 PITR（恢复到指定时间点）
kubectl apply -f - <<EOF
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: postgres-recovery
  namespace: database
spec:
  instances: 3
  storage:
    storageClass: gp3-encrypted
    size: 200Gi
  bootstrap:
    recovery:
      source: postgres-production-backup
      recoveryTarget:
        targetTime: "2026-07-19 14:30:00.000000+00"
  externalClusters:
  - name: postgres-production-backup
    barmanObjectStore:
      destinationPath: "s3://db-backups/production/postgres/"
      s3Credentials:
        accessKeyId:
          name: backup-s3-creds
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: backup-s3-creds
          key: SECRET_ACCESS_KEY
      wal:
        maxParallel: 4
EOF
```

### 备份验证自动化

```yaml
# 🟡 中风险：备份验证 CronJob（定期恢复验证）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-verification
  namespace: database
spec:
  schedule: "0 6 * * 1"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      activeDeadlineSeconds: 3600
      template:
        spec:
          restartPolicy: Never
          containers:
          - name: verify
            image: postgres:16-alpine
            command:
            - /bin/sh
            - -c
            - |
              set -e
              echo "=== Backup Verification Started ==="

              # 1. 恢复最新备份到临时实例
              echo "Restoring latest backup..."
              pg_basebackup -h postgres-primary.database.svc -U backup_user -D /tmp/verify-data -Ft -Xs -P

              # 2. 启动临时 PostgreSQL
              echo "Starting temporary instance..."
              initdb -D /tmp/verify-data 2>/dev/null || true
              pg_ctl -D /tmp/verify-data start -w

              # 3. 执行验证查询
              echo "Running verification queries..."
              psql -h /tmp -U postgres -c "SELECT count(*) FROM pg_stat_user_tables;" > /tmp/verify-result.txt
              psql -h /tmp -U postgres -c "SELECT pg_is_in_recovery();" >> /tmp/verify-result.txt

              # 4. 数据完整性检查
              psql -h /tmp -U postgres -c "SELECT tablename, n_live_tup FROM pg_stat_user_tables ORDER BY n_live_tup DESC LIMIT 10;" >> /tmp/verify-result.txt

              # 5. 清理
              pg_ctl -D /tmp/verify-data stop -w
              rm -rf /tmp/verify-data

              echo "=== Verification Passed ==="
              cat /tmp/verify-result.txt
            resources:
              requests:
                cpu: "1"
                memory: 1Gi
              limits:
                cpu: "2"
                memory: 2Gi
```

### 备份状态检查

```bash
# 🟢 低风险：查看 CloudNativePG 备份历史
kubectl get backup -n database -l cnpg.io/cluster=postgres-production --sort-by='.status.startedAt'

# 🟢 低风险：查看最近备份详情
kubectl describe backup -n database $(kubectl get backup -n database -l cnpg.io/cluster=postgres-production -o jsonpath='{.items[-1].metadata.name}')

# 🟢 低风险：WAL-G 备份列表
kubectl exec -n database postgres-primary-0 -- wal-g backup-list

# 🟢 低风险：检查 WAL 归档状态
kubectl exec -n database postgres-primary-0 -- \
  psql -U postgres -c "SELECT * FROM pg_stat_archiver;"
```

## 故障排查

### 备份失败

```bash
# 🟢 低风险：查看备份 Job 日志
kubectl logs -n database job/walg-backup-28123456 --tail=100

# 🟢 低风险：检查 S3 连通性
kubectl exec -n database postgres-primary-0 -- \
  aws s3 ls s3://db-backups/production/ --endpoint-url https://s3.amazonaws.com

# 🟢 低风险：检查磁盘空间（备份需要临时空间）
kubectl exec -n database postgres-primary-0 -- df -h /var/lib/postgresql/data
```

**常见失败原因**：
1. **S3 凭证过期**：检查 Secret 中的 Access Key 是否有效
2. **磁盘空间不足**：XtraBackup 需要与数据等量的临时空间
3. **网络超时**：大表备份上传 S3 超时，增大 `activeDeadlineSeconds`
4. **权限不足**：备份用户缺少 `REPLICATION CLIENT` 或 `pg_read_all_data` 权限
5. **WAL 归档积压**：`pg_stat_archiver.last_failed_wal` 非空表示归档失败

### 恢复不一致

**现象**：PITR 恢复后数据与预期不符。

排查步骤：
1. 确认恢复目标时间点是否正确（注意时区）
2. 检查 WAL 归档是否连续（无缺失段）
3. 验证 `recovery_target_timeline` 设置
4. 检查是否存在时间线分叉（timeline switch）

```bash
# 🟢 低风险：检查 WAL 归档连续性
kubectl exec -n database postgres-primary-0 -- \
  psql -U postgres -c "SELECT last_archived_wal, last_failed_wal, archived_count, failed_count FROM pg_stat_archiver;"

# 🟢 低风险：查看时间线历史
kubectl exec -n database postgres-primary-0 -- \
  ls -la /var/lib/postgresql/data/pg_wal/*.history
```

## 最佳实践

1. **3-2-1 备份原则**：3 份副本、2 种介质、1 份异地（S3 跨区域复制）
2. **PITR 必配**：生产数据库必须启用 WAL 归档，RPO 目标 < 5 分钟
3. **备份验证**：每周自动恢复验证，未经验证的备份等于没有备份
4. **加密存储**：备份文件启用 AES-256 加密（S3 SSE 或 pgBackRest 内置加密）
5. **保留策略**：全量保留 30 天，WAL 保留 7 天，逻辑备份保留 90 天
6. **资源隔离**：备份 Job 使用独立 Node Pool 或 ResourceQuota，避免影响在线服务
7. **监控告警**：备份成功/失败、WAL 归档延迟、备份大小异常均需告警，接入 [[可观测性/]]
8. **恢复演练**：每季度执行一次完整恢复演练，记录 RTO 实际值，参考 [[可靠性/备份恢复/]]
9. **Operator 集成**：优先使用 CloudNativePG / Zalando Operator 内置备份能力，参考 [[数据库中间件/Operator管理/]]

## Related

- [[可靠性/备份恢复/]]
- [[数据库中间件/数据库/]]
- [[可观测性/]]
- [[数据库中间件/Operator管理/]]
- [[数据库中间件/新型数据库/07-database-observability-pmm.md]]
