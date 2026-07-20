---
title: "零停机数据库迁移模式"
description: "生产级零停机数据库迁移：Expand-Contract 模式、在线 DDL、双写策略、数据校验与回滚方案"
summary: "覆盖 Kubernetes 环境下数据库 Schema 迁移的完整实践，包括 Expand-Contract 渐进模式、在线 DDL 工具、双写一致性保证、数据校验与对账、回滚策略设计，以及迁移过程中的应用兼容性管理。"
category: 应用模式
tags:
- patterns
- database
- migration
- zero-downtime
- ddl
- expand-contract
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 应用开发者
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- "数据库迁移如何做到零停机"
- "Expand-Contract 模式怎么实施"
- "在线 DDL 和数据双写策略"
trigger_keywords:
- 数据库迁移
- 零停机
- Expand-Contract
- 在线 DDL
- 双写
- Schema 变更
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

# 零停机数据库迁移模式

> **适用范围**: Kubernetes v1.28–v1.32 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

## 概述

数据库 Schema 变更是生产环境最高风险的变更类型之一。与代码回滚不同，数据库迁移一旦执行，回滚往往意味着数据丢失。传统的"停机维护窗口"模式在 7×24 在线服务中已不可接受。零停机数据库迁移的核心思想是：通过多步骤渐进式变更，确保在任何时刻应用都能正常工作，任何单步失败都可以安全回退。

本文覆盖 Expand-Contract（扩展-收缩）模式、在线 DDL 工具、双写策略、数据校验和回滚方案，为 Kubernetes 环境下的数据库迁移提供完整工程实践。相关内容可参见 [[release-change-management-patterns]]、[[stateful-app-patterns]]、[[application-runbooks]]。

---

## 模式定义与适用场景

### 迁移模式对比

| 模式 | 停机时间 | 复杂度 | 回滚难度 | 适用场景 | 典型工具 |
|------|---------|--------|---------|---------|---------|
| **停机迁移** | 分钟-小时 | 低 | 低（备份恢复） | 内部系统、维护窗口 | pg_restore, mysqldump |
| **Expand-Contract** | 零 | 高 | 中（逐步回退） | 在线服务 Schema 变更 | Flyway, Liquibase |
| **在线 DDL** | 零（或极短） | 中 | 中 | 大表加列/加索引 | gh-ost, pt-osc |
| **双写迁移** | 零 | 极高 | 高 | 数据库引擎/实例迁移 | 应用层双写 |
| **蓝绿数据库** | 秒级切换 | 高 | 低（切回） | 大版本升级 | 逻辑复制 |

### Expand-Contract 四阶段模型

```
阶段 1: Expand（扩展）
  - 添加新列/新表（不删除旧结构）
  - 应用同时写新旧结构
  - 旧应用完全兼容

阶段 2: Migrate（迁移）
  - 后台任务将旧数据填充到新结构
  - 数据校验确认一致性
  - 新旧数据并存

阶段 3: Contract（收缩）
  - 应用停止写旧结构
  - 验证无流量访问旧结构
  - 删除旧列/旧表

阶段 4: Cleanup（清理）
  - 移除兼容性代码
  - 更新文档
  - 归档迁移记录
```

---

## 架构设计

### 零停机迁移流水线

```
┌─────────────────────────────────────────────────────────────┐
│                    迁移编排层                                  │
│  Argo Workflows / CI Pipeline                                │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐         │
│  │Pre-  │─▶│Expand│─▶│Migrate│─▶│Contract│─▶│Post- │        │
│  │Check │  │ DDL  │  │ Data │  │  DDL   │  │Check │        │
│  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘         │
├─────────────────────────────────────────────────────────────┤
│                    应用层                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  v1 (写旧)  →  v2 (双写)  →  v3 (写新)  →  v4 (清理) │    │
│  └─────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                    数据层                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │ 旧表/列   │  │ 新表/列   │  │ 校验任务  │                  │
│  │(保留)    │  │(新增)    │  │(对账)    │                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
└─────────────────────────────────────────────────────────────┘
```

### 应用版本与 Schema 兼容矩阵

| 应用版本 | 旧 Schema | 新 Schema | 读 | 写 | 阶段 |
|---------|-----------|-----------|----|----|------|
| v1 (当前) | 存在 | 不存在 | 旧 | 旧 | 迁移前 |
| v2 (双写) | 存在 | 存在 | 旧 | 旧+新 | Expand |
| v3 (切读) | 存在 | 存在 | 新 | 新 | Migrate 完成 |
| v4 (清理) | 删除 | 存在 | 新 | 新 | Contract |

---

## K8s 实现

### 迁移 Job（Expand 阶段）

```yaml
# 🔴 高风险：DDL 变更直接影响生产数据库结构
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-expand-001
  namespace: database-ops
  labels:
    app.kubernetes.io/name: db-migration
    kudig.io/migration-phase: expand
    kudig.io/migration-id: "MIG-2026-0719-001"
  annotations:
    kudig.io/change-id: "CHG-2026-0719-003"
    kudig.io/rollback-plan: "kubectl apply -f rollback-expand-001.yaml"
    kudig.io/approved-by: "dba-lead"
spec:
  backoffLimit: 0  # DDL 不自动重试，失败需人工介入
  activeDeadlineSeconds: 3600  # 1 小时超时
  ttlSecondsAfterFinished: 86400
  template:
    metadata:
      labels:
        app.kubernetes.io/name: db-migration
    spec:
      restartPolicy: Never
      priorityClassName: database-ops
      # 初始化容器：预检查
      initContainers:
        - name: pre-check
          image: registry.internal/db-tools/migration-checker:v1.2.0
          command: ["/app/pre-check"]
          args:
            - "--migration=expand-001"
            - "--check=lock-wait,replication-lag,disk-space"
            - "--max-replication-lag=5s"
            - "--min-disk-free=20Gi"
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: db-admin-credentials
                  key: url
          resources:
            requests:
              cpu: "100m"
              memory: "128Mi"
            limits:
              cpu: "500m"
              memory: "256Mi"
      containers:
        - name: migrate
          image: registry.internal/db-tools/flyway:v10.8.0
          command: ["flyway"]
          args:
            - "-url=$(DATABASE_URL)"
            - "-locations=filesystem:/migrations/expand"
            - "-outOfOrder=false"
            - "-validateOnMigrate=true"
            - "migrate"
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: db-admin-credentials
                  key: url
            - name: FLYWAY_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: db-admin-credentials
                  key: password
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
            limits:
              cpu: "1"
              memory: "1Gi"
          volumeMounts:
            - name: migrations
              mountPath: /migrations
      volumes:
        - name: migrations
          configMap:
            name: db-migration-expand-001
---
# 迁移 SQL
apiVersion: v1
kind: ConfigMap
metadata:
  name: db-migration-expand-001
  namespace: database-ops
data:
  V20260719_001__expand_add_new_columns.sql: |
    -- Expand 阶段：只添加，不删除
    -- 添加新列（允许 NULL，不阻塞写入）
    ALTER TABLE orders ADD COLUMN customer_email VARCHAR(255) DEFAULT NULL;
    ALTER TABLE orders ADD COLUMN shipping_address JSONB DEFAULT NULL;
    
    -- 添加新索引（CONCURRENTLY 不锁表）
    CREATE INDEX CONCURRENTLY idx_orders_customer_email 
      ON orders(customer_email) WHERE customer_email IS NOT NULL;
    
    -- 添加新表（不影响现有表）
    CREATE TABLE IF NOT EXISTS order_addresses (
      id BIGSERIAL PRIMARY KEY,
      order_id BIGINT NOT NULL REFERENCES orders(id),
      address_type VARCHAR(20) NOT NULL,
      address_data JSONB NOT NULL,
      created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
      updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
    );
    
    CREATE INDEX idx_order_addresses_order_id ON order_addresses(order_id);
```

### 数据回填 Job（Migrate 阶段）

```yaml
# 🔴 高风险：批量数据修改，需控制速率避免影响在线服务
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-backfill-001
  namespace: database-ops
  labels:
    kudig.io/migration-phase: migrate
    kudig.io/migration-id: "MIG-2026-0719-001"
spec:
  backoffLimit: 3
  activeDeadlineSeconds: 43200  # 最长 12 小时
  parallelism: 1  # 单实例，避免并发冲突
  completions: 1
  template:
    spec:
      restartPolicy: OnFailure
      priorityClassName: batch-low  # 低优先级，不抢占在线服务
      containers:
        - name: backfill
          image: registry.internal/db-tools/data-backfill:v1.0.0
          command: ["/app/backfill"]
          args:
            - "--source-table=customers"
            - "--target-table=orders"
            - "--batch-size=1000"
            - "--batch-interval=500ms"  # 每批间隔 500ms，控制 DB 压力
            - "--max-rows-per-second=2000"
            - "--resume-from-checkpoint=true"
            - "--checkpoint-interval=10000"
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: db-admin-credentials
                  key: url
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
            limits:
              cpu: "2"
              memory: "2Gi"
```

### 数据校验 Job

```yaml
# 🟢 低风险：只读校验，不修改数据
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-verify-001
  namespace: database-ops
  labels:
    kudig.io/migration-phase: verify
    kudig.io/migration-id: "MIG-2026-0719-001"
spec:
  backoffLimit: 2
  activeDeadlineSeconds: 7200
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: verify
          image: registry.internal/db-tools/data-verifier:v1.1.0
          command: ["/app/verify"]
          args:
            - "--check=row-count"
            - "--check=null-violations"
            - "--check=referential-integrity"
            - "--check=checksum-sample"
            - "--sample-rate=0.01"
            - "--report-format=json"
            - "--report-output=/reports/verify-001.json"
            - "--fail-on-mismatch=true"
            - "--max-mismatch-rate=0.001"  # 允许 0.1% 误差（并发写入）
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: db-admin-credentials
                  key: url
          resources:
            requests:
              cpu: "1"
              memory: "1Gi"
            limits:
              cpu: "2"
              memory: "2Gi"
          volumeMounts:
            - name: reports
              mountPath: /reports
      volumes:
        - name: reports
          emptyDir: {}
```

---

## 生产配置示例

### 在线 DDL（gh-ost 大表变更）

```yaml
# 🔴 高风险：大表 DDL 可能持续数小时，需监控复制延迟
apiVersion: batch/v1
kind: Job
metadata:
  name: online-ddl-add-index
  namespace: database-ops
  labels:
    kudig.io/migration-phase: online-ddl
spec:
  backoffLimit: 0
  activeDeadlineSeconds: 86400  # 24 小时超时
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: gh-ost
          image: registry.internal/db-tools/gh-ost:v1.1.6
          command: ["gh-ost"]
          args:
            - "--host=mysql-primary.database.svc"
            - "--database=production"
            - "--table=orders"
            - "--alter=ADD COLUMN priority INT DEFAULT 0, ADD INDEX idx_priority(priority)"
            - "--execute"
            - "--chunk-size=1000"
            - "--max-lag-millis=1500"       # 复制延迟 > 1.5s 暂停
            - "--throttle-query=SELECT IF(COUNT(*) > 100, 1, 0) FROM information_schema.processlist WHERE command != 'Sleep'"
            - "--critical-load=Threads_running=50"  # 负载过高时中止
            - "--max-load=Threads_running=25"       # 负载高时降速
            - "--cut-over=two-step"          # 两步切换，可人工确认
            - "--initially-drop-ghost-table"
            - "--ok-to-drop-table"
            - "--serve-socket-file=/tmp/gh-ost.sock"
          env:
            - name: MYSQL_PWD
              valueFrom:
                secretKeyRef:
                  name: db-admin-credentials
                  key: password
          resources:
            requests:
              cpu: "1"
              memory: "2Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
```

### 双写策略应用配置

```yaml
# 🟡 中风险：双写配置影响数据一致性
apiVersion: v1
kind: ConfigMap
metadata:
  name: order-service-migration-config
  namespace: production
data:
  migration.yaml: |
    # 双写迁移配置
    migration:
      id: "MIG-2026-0719-001"
      phase: "dual-write"  # expand | dual-write | new-only | contract
      
      dual_write:
        enabled: true
        # 写入策略
        primary: "old_schema"     # 主写入目标
        secondary: "new_schema"   # 副写入目标
        # 失败处理
        on_secondary_failure: "log_and_continue"  # 副写失败不阻塞主流程
        on_primary_failure: "fail_request"        # 主写失败则请求失败
        # 一致性
        async_secondary: true     # 副写异步（降低延迟）
        reconciliation_interval: 5m  # 对账间隔
        
      read_strategy:
        source: "old_schema"      # 当前从旧表读
        fallback: "new_schema"    # 旧表无数据时尝试新表
        shadow_read: true         # 影子读：同时读新旧对比（不影响响应）
        shadow_sample_rate: 0.01  # 1% 请求做影子读对比
        
      rollback:
        # 回滚到 expand 阶段（停止双写）
        command: "set phase=expand"
        data_impact: "新表数据停止更新，旧表不受影响"
```

---

## 运维要点

### 迁移前检查清单

```bash
# 🟢 低风险：检查数据库复制延迟
kubectl exec -n database mysql-primary-0 -- \
  mysql -e "SHOW SLAVE STATUS\G" | grep Seconds_Behind_Master

# 🟢 低风险：检查磁盘空间
kubectl exec -n database mysql-primary-0 -- df -h /data

# 🟢 低风险：检查当前活跃连接数
kubectl exec -n database mysql-primary-0 -- \
  mysql -e "SHOW STATUS LIKE 'Threads_connected';"

# 🟢 低风险：检查是否有长事务（可能阻塞 DDL）
kubectl exec -n database mysql-primary-0 -- \
  mysql -e "SELECT * FROM information_schema.innodb_trx WHERE TIME_TO_SEC(TIMEDIFF(NOW(), trx_started)) > 60;"

# 🟢 低风险：检查表大小（评估 DDL 时间）
kubectl exec -n database mysql-primary-0 -- \
  mysql -e "SELECT table_name, table_rows, data_length/1024/1024 AS data_mb FROM information_schema.tables WHERE table_schema='production' ORDER BY data_length DESC LIMIT 10;"
```

### 迁移监控指标

| 指标 | 告警阈值 | 说明 |
|------|---------|------|
| 复制延迟 | > 5s | DDL/回填导致从库延迟 |
| 活跃连接数 | > 80% max | 迁移占用过多连接 |
| 慢查询数量 | > 基线 × 2 | DDL 锁等待 |
| 磁盘使用率 | > 80% | 新表/索引占用空间 |
| 应用错误率 | > 0.1% | 双写/兼容性问题 |
| 回填进度 | 停滞 > 10min | 回填任务卡住 |

### 回滚操作

```bash
# 🔴 高风险：回滚 Expand 阶段（删除新增的列/表）
# 仅在确认新结构无数据写入时执行
kubectl exec -n database mysql-primary-0 -- \
  mysql -e "ALTER TABLE orders DROP COLUMN customer_email, DROP COLUMN shipping_address;"

# 🟡 中风险：停止双写，回退到只写旧表
kubectl patch configmap order-service-migration-config -n production \
  --type merge -p '{"data":{"migration.yaml":"migration:\n  phase: expand\n  dual_write:\n    enabled: false"}}'

# 🟢 低风险：查看迁移 Job 状态
kubectl get jobs -n database-ops -l kudig.io/migration-id=MIG-2026-0719-001

# 🟢 低风险：查看迁移日志
kubectl logs job/db-migration-expand-001 -n database-ops
```

---

## 反模式

### 反模式 1：直接 DROP COLUMN

```sql
-- ❌ 错误：一步到位删除列
ALTER TABLE orders DROP COLUMN old_address;
```

**后果**：如果还有旧版本应用在运行（滚动更新中），立即报错崩溃。且无法回滚（数据已丢失）。

**修正**：Expand-Contract 模式，先停止写入→确认无读取→再删除。参见 [[release-change-management-patterns]]。

### 反模式 2：大表 DDL 不加 CONCURRENTLY

```sql
-- ❌ 错误：对千万行表直接加索引
CREATE INDEX idx_email ON orders(email);
```

**后果**：表级锁持续数十分钟，所有写入阻塞，服务不可用。

**修正**：PostgreSQL 使用 `CREATE INDEX CONCURRENTLY`，MySQL 使用 gh-ost/pt-osc。

### 反模式 3：回填不限速

```sql
-- ❌ 错误：一次性更新全表
UPDATE orders SET new_column = (SELECT ... FROM customers WHERE ...);
```

**后果**：长时间锁表/大量 IO，复制延迟飙升，从库不可用，影响读流量。

**修正**：分批回填（每批 1000 行，间隔 500ms），监控复制延迟，超限暂停。

### 反模式 4：迁移无回滚方案

**后果**：迁移失败后无法回退，只能紧急修复或停机恢复备份。

**修正**：每个迁移步骤必须有对应的回滚 SQL/操作，且在预发布环境验证过。

### 反模式 5：应用代码与 Schema 强耦合

**后果**：Schema 变更必须与应用同时发布，无法独立演进，回滚困难。

**修正**：应用代码兼容新旧 Schema（通过 Feature Flag 控制），Schema 变更和应用发布解耦。参见 [[config-management-feature-flags]]。

---

## Related

- [[release-change-management-patterns]] — 发布变更管理模式
- [[stateful-app-patterns]] — Stateful 应用生产模式
- [[application-runbooks]] — 应用运维 Runbook
- [[config-management-feature-flags]] — 配置管理与 Feature Flag 模式
- [[app-observability-patterns]] — 应用可观测性模式
- [[multi-tenant-app-isolation]] — 多租户应用隔离模式
