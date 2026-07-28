---
title: "数据库迁移发布策略"
description: "数据库 Schema 变更与代码发布解耦：Expand-Contract 模式、在线 DDL、数据回填、回滚策略与零停机迁移实践"
summary: "系统化的数据库迁移发布策略，覆盖 Schema 变更与代码发布的解耦原则、Expand-Contract（并行变更）模式、MySQL/PostgreSQL 在线 DDL 工具、大规模数据回填策略、迁移回滚方案以及 Kubernetes 环境下的迁移编排"
category: 发布变更
tags:
- database-migration
- schema-change
- expand-contract
- online-ddl
- data-backfill
- zero-downtime
- rollback
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "数据库 Schema 变更如何与代码发布解耦"
- "Expand-Contract 模式如何实现零停机数据库迁移"
- "大规模数据回填如何避免影响生产性能"
trigger_keywords:
- 数据库迁移
- schema变更
- expand-contract
- 在线DDL
- 数据回填
- 零停机
prerequisites:
- kubectl-basics
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

# 数据库迁移发布策略

## 概述

数据库 Schema 变更是发布流程中风险最高的操作之一。与应用代码可以通过 [[11-发布变更/04-变更管理/03-change-rollback-playbook.md|回滚]] 快速恢复不同，数据库变更往往不可逆——一旦列被删除或数据被修改，恢复成本极高。在微服务架构中，多个服务共享数据库或存在数据依赖时，Schema 变更的协调复杂度呈指数增长。

本文提供系统化的数据库迁移发布策略，核心原则是：**Schema 变更与代码发布完全解耦，任何时刻数据库 Schema 都兼容所有在线版本的应用代码**。通过 Expand-Contract 模式、在线 DDL 工具和渐进式数据回填，实现真正的零停机数据库迁移。

## 核心概念

### Expand-Contract（并行变更）模式

Expand-Contract 是零停机数据库迁移的核心模式，将一次破坏性变更分解为多个安全的增量步骤：

```
┌─────────────────────────────────────────────────────────────────┐
│              Expand-Contract 模式                                 │
│                                                                   │
│  传统方式（危险）:                                                │
│  ┌──────────┐    ┌──────────┐                                    │
│  │ 旧代码    │───▶│ 新代码    │  ← 部署瞬间旧代码访问新 Schema 会崩溃│
│  │ 旧Schema  │    │ 新Schema  │                                    │
│  └──────────┘    └──────────┘                                    │
│                                                                   │
│  Expand-Contract（安全）:                                         │
│                                                                   │
│  Phase 1: Expand（扩展）                                         │
│  ┌──────────────────────────────────┐                            │
│  │ 添加新列/新表（不删除旧结构）       │                            │
│  │ 旧代码 + 新代码都能正常工作        │                            │
│  └──────────────────────────────────┘                            │
│                    │                                              │
│                    ▼                                              │
│  Phase 2: Migrate（迁移）                                        │
│  ┌──────────────────────────────────┐                            │
│  │ 新代码写入新列（双写）             │                            │
│  │ 后台任务回填历史数据到新列         │                            │
│  │ 旧代码仍然读写旧列（不受影响）     │                            │
│  └──────────────────────────────────┘                            │
│                    │                                              │
│                    ▼                                              │
│  Phase 3: Contract（收缩）                                       │
│  ┌──────────────────────────────────┐                            │
│  │ 确认所有代码已切换到新列           │                            │
│  │ 停止双写                         │                            │
│  │ 删除旧列/旧表（确认无依赖后）     │                            │
│  └──────────────────────────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
```

### 在线 DDL 工具对比

| 工具 | 数据库 | 原理 | 锁表时间 | 磁盘开销 | 适用场景 |
|------|--------|------|---------|---------|---------|
| pt-online-schema-change | MySQL | 创建影子表 + 触发器同步 | 极短（rename） | 2x 表大小 | 大表 ALTER |
| gh-ost (GitHub) | MySQL | 创建影子表 + binlog 同步 | 极短（cut-over） | 2x 表大小 | 超大表、需要暂停/恢复 |
| pg_repack | PostgreSQL | 创建新表 + 触发器 + 日志 | 极短（swap） | 2x 表大小 | 大表 VACUUM/ALTER |
| ALTER TABLE ... ALGORITHM=INPLACE | MySQL 5.6+ | 原生在线 DDL | 取决于操作 | 无额外 | 简单列操作 |
| CREATE INDEX CONCURRENTLY | PostgreSQL | 非阻塞索引构建 | 无 | 索引大小 | 添加索引 |

### 迁移风险评估矩阵

| 操作类型 | 风险等级 | 锁表影响 | 回滚难度 | 推荐方式 |
|---------|---------|---------|---------|---------|
| 添加可空列 | 🟢 低 | 无/极短 | 简单（DROP COLUMN） | 直接 ALTER |
| 添加索引 | 🟡 中 | 取决于工具 | 简单（DROP INDEX） | CONCURRENTLY / gh-ost |
| 修改列类型 | 🔴 高 | 可能全表锁 | 困难 | Expand-Contract |
| 删除列 | 🔴 高 | 短 | 不可逆（数据丢失） | 先停用再删除 |
| 重命名列/表 | 🔴 高 | 短 | 需协调所有引用 | Expand-Contract |
| 大表数据回填 | 🟡 中 | 无（分批） | 可重跑 | 分批 + 限速 |

## 生产部署/实现

### Expand-Contract 完整示例：用户表拆分

将 `users` 表的地址信息拆分到独立的 `user_addresses` 表：

```sql
-- 🔴 高风险：生产数据库 Schema 变更，必须在维护窗口或低峰期执行
-- Phase 1: Expand - 创建新表（不影响现有代码）
CREATE TABLE user_addresses (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    address_type ENUM('shipping', 'billing') NOT NULL DEFAULT 'shipping',
    street VARCHAR(255) NOT NULL,
    city VARCHAR(100) NOT NULL,
    state VARCHAR(100),
    postal_code VARCHAR(20),
    country VARCHAR(100) NOT NULL,
    is_default BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_user_type (user_id, address_type),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Phase 2: Migrate - 分批回填数据（限速执行）
-- 使用存储过程分批迁移，每批 1000 条，间隔 100ms
DELIMITER //
CREATE PROCEDURE backfill_user_addresses()
BEGIN
    DECLARE v_batch_size INT DEFAULT 1000;
    DECLARE v_last_id BIGINT DEFAULT 0;
    DECLARE v_rows_affected INT DEFAULT 1;

    WHILE v_rows_affected > 0 DO
        INSERT INTO user_addresses (user_id, address_type, street, city, state, postal_code, country, is_default)
        SELECT id, 'shipping', shipping_street, shipping_city, shipping_state,
               shipping_postal_code, shipping_country, TRUE
        FROM users
        WHERE id > v_last_id
          AND shipping_street IS NOT NULL
          AND id NOT IN (SELECT user_id FROM user_addresses WHERE address_type = 'shipping')
        ORDER BY id
        LIMIT v_batch_size;

        SET v_rows_affected = ROW_COUNT();
        SET v_last_id = (SELECT COALESCE(MAX(user_id), 0) FROM user_addresses WHERE address_type = 'shipping');

        -- 限速：避免对生产查询造成压力
        DO SLEEP(0.1);
    END WHILE;
END //
DELIMITER ;

-- 执行回填
CALL backfill_user_addresses();

-- Phase 3: Contract（在所有代码切换完成后执行）
-- 确认无代码引用旧列后：
-- ALTER TABLE users DROP COLUMN shipping_street;
-- ALTER TABLE users DROP COLUMN shipping_city;
-- ... (逐列删除)
```

### Kubernetes 迁移 Job 编排

使用 K8s Job 编排数据库迁移，确保迁移在应用部署前完成：

```yaml
# 🔴 高风险：数据库迁移 Job，执行前必须备份
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-expand-v2-5
  namespace: production
  labels:
    app.kubernetes.io/component: db-migration
    migration-phase: expand
    migration-version: v2.5
  annotations:
    migration.kudig.io/backup-required: "true"
    migration.kudig.io/rollback-plan: "DROP TABLE user_addresses"
spec:
  backoffLimit: 1
  activeDeadlineSeconds: 3600
  ttlSecondsAfterFinished: 86400
  template:
    metadata:
      labels:
        app.kubernetes.io/component: db-migration
    spec:
      restartPolicy: Never
      initContainers:
      # 等待数据库就绪
      - name: wait-for-db
        image: registry.internal/db-tools:v1.2.0
        command:
        - /bin/sh
        - -c
        - |
          until mysqladmin ping -h $DB_HOST -u $DB_USER -p$DB_PASSWORD --silent; do
            echo "Waiting for database..."
            sleep 5
          done
          echo "Database is ready"
        env:
        - name: DB_HOST
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: host
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: password
      containers:
      - name: migrate
        image: registry.internal/db-migrations:v2.5.0
        command:
        - /bin/sh
        - -c
        - |
          set -e
          echo "Starting migration: expand phase"

          # 执行迁移脚本
          /migrations/run.sh --phase expand --version v2.5

          # 验证迁移结果
          /migrations/verify.sh --phase expand --version v2.5

          echo "Migration completed successfully"
        env:
        - name: DB_HOST
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: host
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: password
        - name: MIGRATION_BATCH_SIZE
          value: "1000"
        - name: MIGRATION_SLEEP_MS
          value: "100"
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

### gh-ost 在线 DDL 执行

对于大表的 Schema 变更，使用 gh-ost 避免锁表：

```yaml
# 🔴 高风险：大表在线 DDL，需要 2x 磁盘空间
apiVersion: batch/v1
kind: Job
metadata:
  name: gh-ost-add-index-orders
  namespace: production
  labels:
    app.kubernetes.io/component: online-ddl
spec:
  backoffLimit: 0
  activeDeadlineSeconds: 86400
  template:
    spec:
      restartPolicy: Never
      containers:
      - name: gh-ost
        image: registry.internal/gh-ost:v1.1.6
        command:
        - /bin/sh
        - -c
        - |
          gh-ost \
            --host=$DB_HOST \
            --user=$DB_USER \
            --password=$DB_PASSWORD \
            --database=production_db \
            --table=orders \
            --alter="ADD INDEX idx_created_status (created_at, status)" \
            --max-load="Threads_running=50,Threads_connected=200" \
            --critical-load="Threads_running=100,Threads_connected=500" \
            --chunk-size=1000 \
            --max-lag-millis=1500 \
            --throttle-control-replicas="$REPLICA_HOST" \
            --allow-on-master \
            --initially-drop-ghost-table \
            --initially-drop-old-table \
            --ok-to-drop-table \
            --exact-rowcount \
            --concurrent-rowcount \
            --default-retries=3 \
            --verbose \
            --execute
        env:
        - name: DB_HOST
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: host
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: password
        - name: REPLICA_HOST
          value: "mysql-replica-0.mysql-replica.production.svc"
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: "2"
            memory: 2Gi
```

### 迁移回滚策略

```yaml
# 🔴 高风险：回滚操作可能导致数据丢失
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-rollback-v2-5
  namespace: production
  labels:
    app.kubernetes.io/component: db-migration
    migration-phase: rollback
spec:
  backoffLimit: 0
  template:
    spec:
      restartPolicy: Never
      containers:
      - name: rollback
        image: registry.internal/db-migrations:v2.5.0
        command:
        - /bin/sh
        - -c
        - |
          set -e
          echo "=== ROLLBACK: v2.5 expand phase ==="

          # Step 1: 验证回滚安全性
          echo "Checking if new table has been written to by new code..."
          NEW_WRITES=$(mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -N -e "
            SELECT COUNT(*) FROM user_addresses
            WHERE created_at > '$(date -d '1 hour ago' +%Y-%m-%d\ %H:%M:%S)'
            AND user_id NOT IN (SELECT id FROM users WHERE shipping_street IS NOT NULL)
          ")

          if [ "$NEW_WRITES" -gt 0 ]; then
            echo "WARNING: $NEW_WRITES new writes detected in user_addresses"
            echo "These records will be lost. Proceeding with rollback..."
          fi

          # Step 2: 执行回滚（删除新表）
          mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
            DROP PROCEDURE IF EXISTS backfill_user_addresses;
            DROP TABLE IF EXISTS user_addresses;
          "

          # Step 3: 验证回滚结果
          TABLE_EXISTS=$(mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -N -e "
            SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema = 'production_db' AND table_name = 'user_addresses'
          ")

          if [ "$TABLE_EXISTS" -eq 0 ]; then
            echo "Rollback successful: user_addresses table dropped"
          else
            echo "ERROR: Rollback failed, table still exists"
            exit 1
          fi
        env:
        - name: DB_HOST
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: host
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: password
```

## 运维操作

### 迁移前检查

```bash
# 🟢 低风险：只读检查
# 检查表大小和行数（评估迁移时间）
mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
  SELECT table_name, table_rows,
    ROUND(data_length/1024/1024, 2) AS data_mb,
    ROUND(index_length/1024/1024, 2) AS index_mb
  FROM information_schema.tables
  WHERE table_schema = 'production_db'
  ORDER BY data_length DESC
  LIMIT 20;
"

# 检查当前活跃连接数（评估锁表影响）
mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
  SHOW STATUS LIKE 'Threads_connected';
  SHOW STATUS LIKE 'Threads_running';
"

# 检查主从复制延迟（gh-ost 需要）
mysql -h $REPLICA_HOST -u $DB_USER -p$DB_PASSWORD -e "
  SHOW SLAVE STATUS\G
" | grep "Seconds_Behind_Master"

# 检查磁盘空间（在线 DDL 需要 2x 表大小）
kubectl exec -n production statefulset/mysql-primary -- df -h /var/lib/mysql
```

### 迁移进度监控

```bash
# 🟢 低风险：只读监控
# 监控 gh-ost 进度
kubectl logs -n production job/gh-ost-add-index-orders -f

# 监控回填进度
mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
  SELECT
    (SELECT COUNT(*) FROM user_addresses) AS migrated_rows,
    (SELECT COUNT(*) FROM users WHERE shipping_street IS NOT NULL) AS total_rows,
    ROUND((SELECT COUNT(*) FROM user_addresses) * 100.0 /
      NULLIF((SELECT COUNT(*) FROM users WHERE shipping_street IS NOT NULL), 0), 2) AS progress_pct;
"

# 监控迁移期间的数据库性能
watch -n 5 'mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
  SHOW STATUS LIKE \"Threads_running\";
  SHOW STATUS LIKE \"Innodb_rows_read\";
  SHOW STATUS LIKE \"Innodb_rows_inserted\";
"'
```

### 迁移后验证

```bash
# 🟢 低风险：只读验证
# 数据一致性校验
mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
  SELECT COUNT(*) AS mismatches
  FROM users u
  LEFT JOIN user_addresses ua ON u.id = ua.user_id AND ua.address_type = 'shipping'
  WHERE u.shipping_street IS NOT NULL
    AND (ua.street IS NULL OR ua.street != u.shipping_street);
"

# 检查新索引是否生效
mysql -h $DB_HOST -u $DB_USER -p$DB_PASSWORD -e "
  EXPLAIN SELECT * FROM orders WHERE created_at > '2026-07-01' AND status = 'pending';
"

# 验证应用连接正常
kubectl logs -n production deployment/payment-service --tail=20 | grep -i "database\|connection\|error"
```

## 故障排查

### 迁移 Job 失败

```bash
# 🟢 低风险：只读诊断
# 查看迁移 Job 状态和事件
kubectl describe job db-migration-expand-v2-5 -n production
kubectl logs job/db-migration-expand-v2-5 -n production --tail=100

# 检查是否因超时被终止
kubectl get events -n production --field-selector involvedObject.name=db-migration-expand-v2-5

# 检查数据库连接是否中断
kubectl exec -n production deployment/payment-service -- \
  mysqladmin ping -h $DB_HOST -u $DB_USER -p$DB_PASSWORD
```

### gh-ost 暂停与恢复

```bash
# 🟡 中风险：暂停 gh-ost 会延长迁移窗口
# 暂停 gh-ost（当数据库负载过高时）
echo "throttle" > /tmp/gh-ost.sock

# 或通过 gh-ost 的 Unix socket 控制
echo throttle | nc -U /tmp/gh-ost.sock

# 恢复 gh-ost
echo "no-throttle" | nc -U /tmp/gh-ost.sock

# 紧急取消 gh-ost（保留影子表，不执行 cut-over）
echo "panic" | nc -U /tmp/gh-ost.sock
```

### 复制延迟导致迁移阻塞

```bash
# 🟢 低风险：只读诊断
# 检查复制延迟
mysql -h $REPLICA_HOST -u $DB_USER -p$DB_PASSWORD -e "SHOW SLAVE STATUS\G" | \
  grep -E "Seconds_Behind_Master|Slave_IO_Running|Slave_SQL_Running"

# 如果延迟持续增大，考虑临时调整 gh-ost 限速
# 降低 chunk-size 和 max-load 阈值
```

## 最佳实践

### 迁移流程规范

1. **所有 Schema 变更必须经过 Expand-Contract 分解**：禁止直接执行破坏性 DDL（DROP COLUMN、RENAME TABLE）。

2. **迁移脚本版本化管理**：使用 Flyway/Liquibase 管理迁移脚本，与代码仓库同步版本。

3. **迁移前必须备份**：通过 [[12-可靠性/01-备份恢复/index|01-备份恢复]] 流程确保有可用备份。

4. **大表操作使用在线 DDL 工具**：超过 100 万行的表禁止直接 ALTER TABLE。

5. **回填任务必须限速**：每批 1000-5000 行，间隔 50-200ms，避免影响在线查询。

### 与 CI/CD 集成

数据库迁移应集成到 [[11-发布变更/01-GitOps/08-cicd-pipeline-patterns.md|CI/CD 流水线]] 中：
- PR 阶段：自动检测 Schema 变更，运行迁移脚本的 dry-run
- 部署阶段：先执行 Expand 迁移，再部署新代码
- 验证阶段：运行数据一致性校验
- 清理阶段：确认稳定后执行 Contract 迁移

### 多服务协调

当多个服务共享数据库时，Schema 变更需要协调发布顺序：
1. 先部署写入新列的服务（Expand 阶段）
2. 再部署读取新列的服务
3. 最后停止写入旧列并清理（Contract 阶段）

与 [[11-发布变更/04-变更管理/01-change-window-and-approval.md|变更窗口与审批]] 流程集成，确保跨团队协调。

## Related

- [[11-发布变更/04-变更管理/03-change-rollback-playbook.md|变更回滚手册]]
- [[11-发布变更/04-变更管理/01-change-window-and-approval.md|变更窗口与审批]]
- [[11-发布变更/01-GitOps/08-cicd-pipeline-patterns.md|CI/CD 流水线模式]]
- [[11-发布变更/04-变更管理/07-rollback-automation-patterns.md|回滚自动化模式]]
- [[12-可靠性/01-备份恢复/index|01-备份恢复]]
- [[11-发布变更/01-GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts 渐进式交付]]
- [[11-发布变更/04-变更管理/02-canary-release-strategy.md|金丝雀发布策略]]
