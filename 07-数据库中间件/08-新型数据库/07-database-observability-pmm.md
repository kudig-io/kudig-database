---
title: "数据库可观测性（PMM/监控体系）"
description: "覆盖 Percona PMM 部署及 PostgreSQL/MySQL 深度监控、慢查询分析与告警设计"
summary: "数据库监控四维度（性能/可用性/容量/安全），PMM Server+Client 部署，pg_stat_statements 深度分析，MySQL Performance Schema，慢查询优化，告警规则设计，Prometheus/Grafana 集成，监控数据缺失与告警风暴排查"
category: 数据库中间件
tags:
- database
- observability
- pmm
- monitoring
- postgresql
- mysql
- prometheus
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
- "数据库监控如何搭建"
- "PMM 如何在 K8s 上部署"
- "慢查询如何分析优化"
trigger_keywords:
- 数据库监控
- PMM
- pg_stat_statements
- Performance Schema
- 慢查询
- 告警规则
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

# 数据库可观测性（PMM/监控体系）

## 概述

数据库可观测性是保障数据服务 SLA 的基石。不同于应用层监控，数据库监控需要深入到查询引擎、存储引擎、锁机制和复制拓扑等内部维度。Percona Monitoring and Management（PMM）是开源数据库监控的事实标准，支持 PostgreSQL、MySQL、MongoDB 的深度可观测。

本文覆盖数据库监控体系设计、PMM 在 K8s 上的部署、查询性能分析工具和告警策略，是 [[09-可观测性/index.md|09-可观测性]] 体系在数据库层的延伸实践。

## 架构与核心概念

### 数据库监控四维度

| 维度 | 关键指标 | 告警阈值示例 | 工具 |
|------|---------|------------|------|
| **性能** | QPS/TPS、查询延迟 P99、缓存命中率 | P99 > 500ms | pg_stat_statements / Performance Schema |
| **可用性** | 连接数、复制延迟、主从切换 | 复制延迟 > 10s | pg_stat_replication / SHOW SLAVE STATUS |
| **容量** | 磁盘使用、表膨胀、WAL 增长 | 磁盘 > 80% | pg_stat_user_tables / information_schema |
| **安全** | 异常登录、权限变更、审计日志 | 非白名单 IP 连接 | pg_audit / MySQL Audit Plugin |

### PMM 架构

- **PMM Server**：
  - VictoriaMetrics（时序存储，替代 Prometheus）
  - Grafana（可视化面板）
  - Alertmanager（告警路由）
  - ClickHouse（Query Analytics 存储）
- **PMM Client（pmm-agent）**：
  - node_exporter（系统指标）
  - postgres_exporter / mysqld_exporter（数据库指标）
  - Query Analytics Agent（慢查询采集）

### 核心监控指标

**PostgreSQL 关键指标：**

| 指标 | 来源 | 含义 | 健康阈值 |
|------|------|------|---------|
| pg_stat_activity.count | pg_stat_activity | 活跃连接数 | < max_connections × 0.8 |
| pg_stat_database.blks_hit_rate | pg_stat_database | 缓存命中率 | > 99% |
| pg_stat_replication.replay_lag | pg_stat_replication | 复制延迟 | < 1s |
| pg_stat_statements.mean_exec_time | pg_stat_statements | 平均查询时间 | < 100ms |
| pg_locks.waiting | pg_locks | 锁等待数 | = 0 |
| pg_stat_bgwriter.buffers_backend | pg_stat_bgwriter | 后端直接写缓冲 | 趋势下降 |

## 生产部署

### PMM Server 部署

```yaml
# 🟡 中风险：部署 PMM Server（含持久化存储）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: pmm-server
  namespace: monitoring
spec:
  serviceName: pmm-server
  replicas: 1
  selector:
    matchLabels:
      app: pmm-server
  template:
    metadata:
      labels:
        app: pmm-server
    spec:
      containers:
      - name: pmm-server
        image: percona/pmm-server:2.43.0
        ports:
        - containerPort: 80
          name: http
        - containerPort: 443
          name: https
        resources:
          requests:
            cpu: "2"
            memory: 4Gi
          limits:
            cpu: "4"
            memory: 8Gi
        volumeMounts:
        - name: pmm-data
          mountPath: /srv
        env:
        - name: GF_SECURITY_ADMIN_PASSWORD
          valueFrom:
            secretKeyRef:
              name: pmm-credentials
              key: admin-password
        - name: METRICS_MEMORY_LIMIT
          value: "4294967296"
        - name: DATA_RETENTION
          value: "720h"
        livenessProbe:
          httpGet:
            path: /v1/readyz
            port: 80
          initialDelaySeconds: 60
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /v1/readyz
            port: 80
          initialDelaySeconds: 30
          periodSeconds: 10
  volumeClaimTemplates:
  - metadata:
      name: pmm-data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3-encrypted
      resources:
        requests:
          storage: 100Gi
---
apiVersion: v1
kind: Service
metadata:
  name: pmm-server
  namespace: monitoring
spec:
  selector:
    app: pmm-server
  ports:
  - name: http
    port: 80
    targetPort: 80
  - name: https
    port: 443
    targetPort: 443
  type: ClusterIP
```

### PMM Client 注册

```bash
# 🟡 中风险：在 PostgreSQL Pod 中安装并注册 PMM Client
kubectl exec -n database postgres-primary-0 -- bash -c '
  # 安装 pmm2-client
  yum install -y percona-release || apt-get install -y percona-release
  percona-release enable-only pmm2-client release
  yum install -y pmm2-client || apt-get install -y pmm2-client

  # 配置 pmm-agent
  pmm-admin config \
    --server-insecure-tls \
    --server-url=https://pmm-server.monitoring.svc:443 \
    --server-username=admin \
    --server-password="${PMM_PASSWORD}"

  # 添加 PostgreSQL 监控
  pmm-admin add postgresql \
    --username=pmm_monitor \
    --password="${PG_MONITOR_PASSWORD}" \
    --host=127.0.0.1 \
    --port=5432 \
    --query-source=pgstatstatements \
    --disable-collectors=heartbeat

  # 添加系统监控
  pmm-admin add linux
'
```

### pg_stat_statements 配置

```yaml
# 🟡 中风险：PostgreSQL 配置 pg_stat_statements（通过 ConfigMap）
apiVersion: v1
kind: ConfigMap
metadata:
  name: postgres-monitoring-config
  namespace: database
data:
  monitoring.conf: |
    # pg_stat_statements 配置
    shared_preload_libraries = 'pg_stat_statements,auto_explain'
    pg_stat_statements.max = 10000
    pg_stat_statements.track = all
    pg_stat_statements.track_utility = off
    pg_stat_statements.track_planning = on
    pg_stat_statements.save = on

    # auto_explain（自动记录慢查询执行计划）
    auto_explain.log_min_duration = '1s'
    auto_explain.log_analyze = on
    auto_explain.log_buffers = on
    auto_explain.log_format = json

    # 日志配置
    log_min_duration_statement = '500ms'
    log_checkpoints = on
    log_lock_waits = on
    log_temp_files = 0
```

## 运维操作

### 慢查询分析

```sql
-- 🟢 低风险：PostgreSQL Top 慢查询（pg_stat_statements）
SELECT
  queryid,
  LEFT(query, 100) AS query_preview,
  calls,
  round(mean_exec_time::numeric, 2) AS avg_ms,
  round(total_exec_time::numeric, 2) AS total_ms,
  rows,
  round((shared_blks_hit * 100.0 / NULLIF(shared_blks_hit + shared_blks_read, 0))::numeric, 2) AS hit_rate_pct,
  round(temp_blks_written::numeric / NULLIF(calls, 0), 2) AS avg_temp_blocks
FROM pg_stat_statements
ORDER BY mean_exec_time DESC
LIMIT 20;

-- 🟢 低风险：查找全表扫描的大表
SELECT
  schemaname,
  relname,
  seq_scan,
  seq_tup_read,
  idx_scan,
  n_live_tup,
  round(100.0 * idx_scan / NULLIF(seq_scan + idx_scan, 0), 2) AS idx_scan_pct
FROM pg_stat_user_tables
WHERE n_live_tup > 100000
  AND seq_scan > 0
ORDER BY seq_tup_read DESC
LIMIT 20;

-- 🟢 低风险：MySQL 慢查询分析（Performance Schema）
SELECT
  DIGEST_TEXT,
  COUNT_STAR AS exec_count,
  ROUND(AVG_TIMER_WAIT/1000000000, 2) AS avg_ms,
  ROUND(SUM_TIMER_WAIT/1000000000, 2) AS total_ms,
  SUM_ROWS_EXAMINED AS rows_examined,
  SUM_ROWS_SENT AS rows_sent,
  ROUND(SUM_ROWS_EXAMINED / NULLIF(SUM_ROWS_SENT, 0), 2) AS examine_ratio
FROM performance_schema.events_statements_summary_by_digest
ORDER BY AVG_TIMER_WAIT DESC
LIMIT 20;
```

### 告警规则设计

```yaml
# 🟡 中风险：Prometheus 告警规则（数据库核心告警）
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: database-alerts
  namespace: monitoring
spec:
  groups:
  - name: database.availability
    rules:
    - alert: PostgreSQLDown
      expr: pg_up == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "PostgreSQL 实例 {{ $labels.instance }} 不可达"
    - alert: PostgreSQLReplicationLag
      expr: pg_replication_lag_seconds > 10
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "复制延迟 {{ $value }}s 超过阈值"
    - alert: PostgreSQLConnectionsExhausted
      expr: pg_stat_activity_count / pg_settings_max_connections > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "连接使用率 {{ $value | humanizePercentage }} 接近上限"
  - name: database.performance
    rules:
    - alert: PostgreSQLSlowQueries
      expr: rate(pg_stat_statements_mean_exec_time_seconds_sum[5m]) > 0.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "平均查询延迟超过 500ms"
    - alert: PostgreSQLCacheHitRateLow
      expr: pg_stat_database_blks_hit / (pg_stat_database_blks_hit + pg_stat_database_blks_read) < 0.95
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "缓存命中率低于 95%"
  - name: database.capacity
    rules:
    - alert: PostgreSQLDiskSpaceLow
      expr: pg_database_size_bytes / pg_tablespace_size_bytes > 0.85
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "数据库磁盘使用率超过 85%"
    - alert: PostgreSQLTableBloat
      expr: pg_stat_user_tables_n_dead_tup / pg_stat_user_tables_n_live_tup > 0.5
      for: 1h
      labels:
        severity: info
      annotations:
        summary: "表 {{ $labels.relname }} 死元组比例过高，需要 VACUUM"
```

### 与 Prometheus/Grafana 集成

```bash
# 🟢 低风险：验证 postgres_exporter metrics 端点
kubectl exec -n monitoring deploy/prometheus -- \
  wget -qO- http://postgres-exporter.database.svc:9187/metrics | head -20

# 🟢 低风险：检查 PMM 采集的指标
curl -s "http://pmm-server.monitoring.svc/api/v1/metrics" | python3 -m json.tool | head -30
```

## 故障排查

### 监控数据缺失

**现象**：Grafana 面板出现数据断点。

```bash
# 🟢 低风险：检查 pmm-agent 状态
kubectl exec -n database postgres-primary-0 -- pmm-admin status

# 🟢 低风险：检查 exporter 进程
kubectl exec -n database postgres-primary-0 -- ps aux | grep -E "postgres_exporter|node_exporter"

# 🟢 低风险：检查 PMM Server 存储状态
kubectl exec -n monitoring pmm-server-0 -- supervisorctl status

# 🟢 低风险：检查网络连通性
kubectl exec -n database postgres-primary-0 -- \
  curl -sk https://pmm-server.monitoring.svc/v1/readyz
```

**常见原因**：
1. pmm-agent 与 Server 网络不通（DNS/NetworkPolicy）
2. 监控用户权限不足（需要 `pg_monitor` 角色）
3. PMM Server 磁盘满导致 VictoriaMetrics 停止写入
4. Pod 重启后 pmm-agent 未自动重新注册

### 告警风暴

**现象**：短时间内收到大量重复告警。

**解决方案**：
1. 配置 Alertmanager 的 `group_by` 和 `group_wait` 聚合告警
2. 设置合理的 `for` 持续时间避免瞬时抖动触发
3. 使用 `inhibit_rules` 抑制级联告警（如主库 down 时抑制复制延迟告警）
4. 检查是否存在监控目标频繁上下线（flapping）

```yaml
# 🟡 中风险：Alertmanager 告警聚合配置
route:
  group_by: ['alertname', 'instance']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
inhibit_rules:
- source_match:
    alertname: PostgreSQLDown
  target_match:
    alertname: PostgreSQLReplicationLag
  equal: ['instance']
```

## 最佳实践

1. **监控用户最小权限**：创建专用监控用户，PostgreSQL 授予 `pg_monitor` 角色，MySQL 授予 `PROCESS, REPLICATION CLIENT, SELECT` 权限
2. **pg_stat_statements 必开**：这是 PostgreSQL 性能分析的核心扩展，生产环境必须启用
3. **慢查询阈值分级**：500ms 记录日志，1s 记录执行计划，5s 触发告警
4. **定期重置统计**：每周 `SELECT pg_stat_statements_reset()` 避免统计数据膨胀
5. **容量预测**：基于历史增长趋势设置 80%/90% 两级磁盘告警
6. **面板标准化**：建立统一的数据库 Grafana Dashboard 模板，参考 [[09-可观测性/index.md|09-可观测性]] 中的面板设计规范
7. **与备份联动**：监控备份任务状态，参考 [[12-可靠性/01-备份恢复/index.md|01-备份恢复]] 设置备份失败告警
8. **连接池监控**：如使用 PgBouncer/ProxySQL，同步监控连接池指标，参考 [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]

## Related

- [[09-可观测性/index.md|09-可观测性]]
- [[07-数据库中间件/01-数据库/index.md|01-数据库]]
- [[12-可靠性/01-备份恢复/index.md|01-备份恢复]]
- [[07-数据库中间件/08-新型数据库/05-connection-pooling-pgbouncer-proxysql.md]]
- [[07-数据库中间件/05-Operator管理/index.md|05-Operator管理]]
