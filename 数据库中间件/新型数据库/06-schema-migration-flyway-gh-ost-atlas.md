---
title: "数据库 Schema 迁移（Flyway/gh-ost/Atlas）"
description: "覆盖 Flyway、gh-ost、Atlas 等 Schema 迁移工具在 K8s CI/CD 中的零停机变更实践"
summary: "Schema 迁移挑战（零停机/向后兼容），Flyway CI/CD 集成，gh-ost 在线 DDL（MySQL），Atlas 声明式管理，Liquibase 对比，K8s Job 执行迁移，expand-contract 策略，迁移失败回滚与锁等待排查"
category: 数据库中间件
tags:
- database
- schema-migration
- flyway
- gh-ost
- atlas
- ci-cd
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
- "数据库 Schema 迁移如何零停机"
- "Flyway 在 K8s 中如何使用"
- "gh-ost 在线 DDL 原理"
trigger_keywords:
- Schema迁移
- Flyway
- gh-ost
- Atlas
- Liquibase
- 在线DDL
- 零停机
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

# 数据库 Schema 迁移（Flyway/gh-ost/Atlas）

## 概述

数据库 Schema 变更是应用发布中最危险的环节之一：一个错误的 DDL 可能导致表锁、数据丢失或服务中断。在 Kubernetes 微服务架构中，多个服务可能共享数据库，Schema 变更需要向后兼容且零停机。

本文覆盖主流 Schema 迁移工具（Flyway、gh-ost、Atlas、Liquibase）在 K8s CI/CD 管线中的集成实践，包括迁移策略设计、在线 DDL 原理和故障排查。Schema 迁移是 [[数据库中间件/数据库/]] 运维的核心能力，与 [[可靠性/备份恢复/]] 紧密配合确保安全变更。

## 架构与核心概念

### Schema 迁移挑战

| 挑战 | 说明 | 解决思路 |
|------|------|---------|
| 零停机 | 变更期间服务不能中断 | 在线 DDL / expand-contract |
| 向后兼容 | 新旧版本应用同时运行 | 只增不删、分阶段发布 |
| 大表变更 | ALTER TABLE 锁表数小时 | gh-ost / pt-online-schema-change |
| 多环境一致性 | dev/staging/prod Schema 一致 | 版本化迁移脚本 |
| 回滚能力 | 变更失败需快速回退 | 向下迁移脚本 / 快照 |
| 并发安全 | 多实例同时执行迁移 | 分布式锁（flyway_schema_history 表） |

### 工具对比

| 特性 | Flyway | gh-ost | Atlas | Liquibase |
|------|--------|--------|-------|-----------|
| 数据库支持 | PG/MySQL/Oracle 等 20+ | 仅 MySQL | PG/MySQL/SQLite | PG/MySQL/Oracle 等 |
| 迁移方式 | 版本化 SQL 脚本 | 在线 DDL（binlog 复制） | 声明式（desired state） | XML/YAML/SQL 变更集 |
| 大表 DDL | 不支持在线 | 原生支持 | 集成 gh-ost | 不支持在线 |
| CI/CD 集成 | 优秀（CLI/Maven/Gradle） | 命令行 | CLI + GitHub Action | CLI/Maven/Gradle |
| 漂移检测 | 有限 | 无 | 原生支持 | 有限 |
| 回滚 | 手动编写 undo 脚本 | 自动（cut-over） | 声明式 diff | 自动生成 rollback |
| 学习曲线 | 低 | 中 | 中 | 中 |
| 适用场景 | 通用迁移 | MySQL 大表在线变更 | 全生命周期管理 | 企业级合规审计 |

### 迁移策略

**Expand-Contract（扩展-收缩）模式：**

```
阶段 1 - Expand（向后兼容）:
  → 添加新列（不删除旧列）
  → 应用同时写入新旧列
  → 旧版本应用正常运行

阶段 2 - Migrate（数据迁移）:
  → 后台任务将旧列数据复制到新列
  → 验证数据一致性

阶段 3 - Contract（收缩）:
  → 确认所有应用已使用新列
  → 删除旧列
  → 清理临时代码
```

## 生产部署

### Flyway K8s Job 执行迁移

```yaml
# 🔴 高风险：Flyway 迁移 Job（会修改数据库 Schema）
apiVersion: batch/v1
kind: Job
metadata:
  name: flyway-migrate-v2-5-0
  namespace: database
  labels:
    app: flyway
    version: "2.5.0"
  annotations:
    argocd.argoproj.io/hook: PreSync
    argocd.argoproj.io/hook-delete-policy: BeforeHookCreation
spec:
  backoffLimit: 2
  activeDeadlineSeconds: 600
  template:
    metadata:
      labels:
        app: flyway
    spec:
      restartPolicy: Never
      initContainers:
      - name: wait-for-db
        image: busybox:1.36
        command: ['sh', '-c', 'until nc -z postgres-primary.database.svc 5432; do echo waiting for db; sleep 2; done']
      containers:
      - name: flyway
        image: flyway/flyway:10.17-alpine
        args:
        - -url=jdbc:postgresql://postgres-primary.database.svc:5432/myapp
        - -user=${DB_USER}
        - -password=${DB_PASSWORD}
        - -locations=filesystem:/flyway/sql
        - -connectRetries=3
        - -lockRetryCount=5
        - migrate
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: flyway-credentials
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: flyway-credentials
              key: password
        resources:
          requests:
            cpu: "250m"
            memory: 256Mi
          limits:
            cpu: "500m"
            memory: 512Mi
        volumeMounts:
        - name: migrations
          mountPath: /flyway/sql
      volumes:
      - name: migrations
        configMap:
          name: flyway-migrations-v2-5-0
```

### gh-ost 在线 DDL

```yaml
# 🔴 高风险：gh-ost 在线 DDL Job（MySQL 大表变更）
apiVersion: batch/v1
kind: Job
metadata:
  name: gh-ost-add-column-users
  namespace: database
spec:
  backoffLimit: 1
  activeDeadlineSeconds: 7200
  template:
    spec:
      restartPolicy: Never
      containers:
      - name: gh-ost
        image: github/gh-ost:1.1.6
        command:
        - gh-ost
        args:
        - --host=mysql-primary.database.svc
        - --port=3306
        - --database=myapp
        - --table=users
        - --alter=ADD COLUMN phone_verified BOOLEAN DEFAULT FALSE, ADD INDEX idx_phone (phone)
        - --user=${DB_USER}
        - --password=${DB_PASSWORD}
        - --allow-on-master
        - --initially-drop-ghost-table
        - --initially-drop-old-table
        - --ok-to-drop-table
        - --chunk-size=1000
        - --max-lag-millis=1500
        - --throttle-control-replicas=mysql-replica-0.database.svc
        - --critical-load=Threads_running=50
        - --max-load=Threads_running=25
        - --cut-over=atomic
        - --exact-rowcount
        - --concurrent-rowcount
        - --default-retries=3
        - --verbose
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: ghost-credentials
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: ghost-credentials
              key: password
        resources:
          requests:
            cpu: "1"
            memory: 1Gi
          limits:
            cpu: "2"
            memory: 2Gi
```

### Atlas 声明式 Schema 管理

```yaml
# 🟡 中风险：Atlas Schema 声明文件（atlas.hcl）
apiVersion: v1
kind: ConfigMap
metadata:
  name: atlas-schema
  namespace: database
data:
  schema.hcl: |
    schema "myapp" {}

    table "users" {
      schema = schema.myapp
      column "id" {
        type = bigint
        auto_increment = true
      }
      column "email" {
        type = varchar(255)
      }
      column "phone" {
        type = varchar(20)
        null = true
      }
      column "phone_verified" {
        type = boolean
        default = false
      }
      column "created_at" {
        type = timestamp
        default = sql("CURRENT_TIMESTAMP")
      }
      primary_key {
        columns = [column.id]
      }
      index "idx_email" {
        unique = true
        columns = [column.email]
      }
      index "idx_phone" {
        columns = [column.phone]
      }
    }
  atlas.hcl: |
    env "production" {
      src = "file://schema.hcl"
      dev = "docker://mysql/8/dev"
      url = "mysql://${DB_USER}:${DB_PASSWORD}@mysql-primary.database.svc:3306/myapp"
      migration {
        dir = "file://migrations"
        format = atlas
      }
      diff {
        skip {
          drop_schema = true
          drop_table = true
        }
      }
    }
```

## 运维操作

### Flyway 迁移管理

```bash
# 🟢 低风险：查看迁移历史
kubectl exec -n database deploy/flyway-runner -- \
  flyway -url=jdbc:postgresql://postgres-primary:5432/myapp \
  -user="${DB_USER}" -password="${DB_PASSWORD}" info

# 🟡 中风险：修复失败的迁移记录（标记为已解决）
kubectl exec -n database deploy/flyway-runner -- \
  flyway -url=jdbc:postgresql://postgres-primary:5432/myapp \
  -user="${DB_USER}" -password="${DB_PASSWORD}" repair

# 🔴 高风险：回滚到指定版本（需要 undo 脚本或手动操作）
kubectl exec -n database deploy/flyway-runner -- \
  flyway -url=jdbc:postgresql://postgres-primary:5432/myapp \
  -user="${DB_USER}" -password="${DB_PASSWORD}" undo -target=2.4.0
```

### gh-ost 运行时控制

```bash
# 🟢 低风险：查看 gh-ost 进度（通过 Unix Socket）
kubectl exec -n database job/gh-ost-add-column-users -- \
  echo "status" | nc -U /tmp/gh-ost.sock

# 🟡 中风险：暂停 gh-ost（减少主库负载）
kubectl exec -n database job/gh-ost-add-column-users -- \
  echo "throttle" | nc -U /tmp/gh-ost.sock

# 🟡 中风险：恢复 gh-ost
kubectl exec -n database job/gh-ost-add-column-users -- \
  echo "no-throttle" | nc -U /tmp/gh-ost.sock

# 🔴 高风险：立即执行 cut-over（表名切换）
kubectl exec -n database job/gh-ost-add-column-users -- \
  echo "cut-over" | nc -U /tmp/gh-ost.sock
```

## 故障排查

### 迁移失败回滚

**现象**：Flyway Job 失败，`flyway_schema_history` 表中 `success = false`。

```bash
# 🟢 低风险：查看失败详情
kubectl logs -n database job/flyway-migrate-v2-5-0 --tail=50

# 🟢 低风险：检查迁移历史表
kubectl exec -n database postgres-primary-0 -- \
  psql -U postgres -d myapp -c "SELECT version, description, success, installed_on FROM flyway_schema_history ORDER BY installed_rank DESC LIMIT 5;"
```

**解决步骤**：
1. 分析失败原因（语法错误 / 锁超时 / 约束冲突）
2. 手动修复数据库到一致状态
3. 执行 `flyway repair` 清除失败记录
4. 修正迁移脚本后重新执行

### 锁等待

**现象**：DDL 语句长时间不返回，其他查询被阻塞。

```sql
-- 🟢 低风险：PostgreSQL 查看锁等待
SELECT
  blocked_locks.pid AS blocked_pid,
  blocked_activity.query AS blocked_query,
  blocking_locks.pid AS blocking_pid,
  blocking_activity.query AS blocking_query,
  now() - blocked_activity.query_start AS wait_duration
FROM pg_catalog.pg_locks blocked_locks
JOIN pg_catalog.pg_stat_activity blocked_activity ON blocked_activity.pid = blocked_locks.pid
JOIN pg_catalog.pg_locks blocking_locks
  ON blocking_locks.locktype = blocked_locks.locktype
  AND blocking_locks.relation = blocked_locks.relation
  AND blocking_locks.pid != blocked_locks.pid
JOIN pg_catalog.pg_stat_activity blocking_activity ON blocking_activity.pid = blocking_locks.pid
WHERE NOT blocked_locks.granted;

-- 🔴 高风险：终止阻塞查询（确认安全后执行）
SELECT pg_terminate_backend(<blocking_pid>);
```

```sql
-- 🟢 低风险：MySQL 查看锁等待（gh-ost 相关）
SELECT * FROM information_schema.INNODB_LOCK_WAITS;
SELECT * FROM information_schema.PROCESSLIST WHERE Command != 'Sleep' AND Time > 30;
```

### gh-ost 复制延迟过高

```bash
# 🟢 低风险：检查 gh-ost 日志中的延迟信息
kubectl logs -n database job/gh-ost-add-column-users --tail=100 | grep -i "lag\|throttle"

# 🟢 低风险：检查 MySQL 复制延迟
kubectl exec -n database mysql-replica-0 -- \
  mysql -u root -p"${MYSQL_ROOT_PASSWORD}" -e "SHOW SLAVE STATUS\G" | grep Seconds_Behind_Master
```

## 最佳实践

1. **迁移脚本规范**：每个版本一个文件（`V2_5_0__add_phone_column.sql`），禁止修改已执行的脚本
2. **CI/CD 集成**：迁移作为 ArgoCD PreSync Hook 或 Helm pre-install Hook 执行，确保先于应用部署
3. **大表变更**：超过 100 万行的表使用 gh-ost（MySQL）或 `CREATE INDEX CONCURRENTLY`（PostgreSQL）
4. **Expand-Contract**：所有破坏性变更分两阶段发布，确保新旧版本应用兼容
5. **备份先行**：重大变更前执行全量备份，参考 [[可靠性/备份恢复/]] 确认 RPO
6. **锁超时设置**：DDL 前设置 `lock_timeout = '30s'`（PG）或 `lock_wait_timeout = 30`（MySQL），避免无限等待
7. **变更窗口**：大表 DDL 安排在低峰期，gh-ost 设置 `--max-load` 限制负载
8. **审计追踪**：所有迁移通过 Git 版本控制，配合 [[可观测性/]] 记录变更时间线
9. **Operator 集成**：如使用 CloudNativePG 等 Operator，可利用其内置的 Schema 管理能力，参考 [[数据库中间件/Operator管理/]]

## Related

- [[数据库中间件/数据库/]]
- [[可靠性/备份恢复/]]
- [[可观测性/]]
- [[数据库中间件/Operator管理/]]
- [[数据库中间件/新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]
