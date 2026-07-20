---
title: "数据库连接池（PgBouncer/ProxySQL）"
description: "覆盖 PgBouncer 和 ProxySQL 在 Kubernetes 上的连接池部署、参数调优与故障排查"
summary: "连接池必要性（连接开销/资源耗尽），PgBouncer 三种池化模式（transaction/session/statement），ProxySQL 查询路由与读写分离，K8s Sidecar vs 独立 Deployment 模式，参数调优，监控与故障排查"
category: 数据库中间件
tags:
- database
- connection-pool
- pgbouncer
- proxysql
- postgresql
- mysql
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
- "PgBouncer 如何在 K8s 上部署"
- "数据库连接池参数如何调优"
- "ProxySQL 读写分离配置"
trigger_keywords:
- 连接池
- PgBouncer
- ProxySQL
- connection pool
- 读写分离
- 连接泄漏
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

# 数据库连接池（PgBouncer/ProxySQL）

## 概述

数据库连接是昂贵资源：每个 PostgreSQL 连接消耗约 5-10MB 内存（进程模型），MySQL 每连接约 1-3MB。当微服务架构中数十个服务同时连接数据库时，连接数很容易超过数据库的 `max_connections` 限制，导致 "too many connections" 错误或性能急剧下降。

连接池通过复用少量数据库连接服务大量客户端请求，是 [[数据库中间件/数据库/]] 生产运维中不可或缺的中间件层。本文覆盖 PgBouncer（PostgreSQL）和 ProxySQL（MySQL）在 Kubernetes 上的部署、调优和故障排查。

## 架构与核心概念

### 连接池必要性

| 问题 | 无连接池 | 有连接池 |
|------|---------|---------|
| 连接建立开销 | 每次请求 TCP 握手 + 认证（50-200ms） | 复用已有连接（< 1ms） |
| 数据库连接数 | 随客户端线性增长 | 固定上限，可控 |
| 内存消耗 | N 个客户端 × 每连接内存 | 池大小 × 每连接内存 |
| 突发流量 | 可能耗尽 max_connections | 排队等待，平滑削峰 |
| 连接泄漏 | 直接影响数据库 | 池自动回收超时连接 |

### PgBouncer 池化模式

| 模式 | 连接复用时机 | 适用场景 | 限制 |
|------|------------|---------|------|
| **Transaction** | 事务结束后归还 | 大多数 Web 应用（推荐） | 不支持 PREPARE/SET 跨事务 |
| **Session** | 客户端断开后归还 | 需要会话级 SET/PREPARE | 复用率低 |
| **Statement** | 每条 SQL 后归还 | 简单只读查询 | 不支持多语句事务 |

### ProxySQL 核心功能

- **查询路由**：基于规则将查询分发到不同 MySQL 实例（读/写分离）
- **连接复用**：后端连接池（multiplexing）
- **查询缓存**：可选的结果集缓存
- **查询重写**：正则替换 SQL 语句
- **故障检测**：自动剔除不健康后端

### K8s 中的连接池部署模式

| 模式 | 优点 | 缺点 | 适用场景 |
|------|------|------|---------|
| **Sidecar** | 应用无需改配置，localhost 连接 | 每 Pod 一个池，总连接数难控 | 少量大 Pod |
| **独立 Deployment** | 集中管理，连接数可控 | 多一跳网络延迟（< 1ms） | 微服务集群（推荐） |
| **DaemonSet** | 每节点一个池，平衡延迟和复用 | 节点数变化影响总连接 | 大规模集群 |

## 生产部署

### PgBouncer 独立 Deployment

```yaml
# 🟡 中风险：部署 PgBouncer 连接池（独立 Deployment 模式）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: pgbouncer
  namespace: database
  labels:
    app: pgbouncer
spec:
  replicas: 3
  selector:
    matchLabels:
      app: pgbouncer
  template:
    metadata:
      labels:
        app: pgbouncer
    spec:
      containers:
      - name: pgbouncer
        image: edoburu/pgbouncer:1.23.1
        ports:
        - containerPort: 6432
          name: pgbouncer
        env:
        - name: DB_HOST
          value: "postgres-primary.database.svc.cluster.local"
        - name: DB_PORT
          value: "5432"
        - name: DB_NAME
          value: "*"
        - name: POOL_MODE
          value: "transaction"
        - name: MAX_CLIENT_CONN
          value: "1000"
        - name: DEFAULT_POOL_SIZE
          value: "25"
        - name: MIN_POOL_SIZE
          value: "5"
        - name: RESERVE_POOL_SIZE
          value: "5"
        - name: RESERVE_POOL_TIMEOUT
          value: "3"
        - name: SERVER_IDLE_TIMEOUT
          value: "300"
        - name: SERVER_LIFETIME
          value: "3600"
        - name: AUTH_TYPE
          value: "scram-sha-256"
        - name: AUTH_FILE
          value: "/etc/pgbouncer/userlist.txt"
        resources:
          requests:
            cpu: "500m"
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 512Mi
        volumeMounts:
        - name: config
          mountPath: /etc/pgbouncer
        - name: secrets
          mountPath: /etc/pgbouncer/secrets
          readOnly: true
        livenessProbe:
          tcpSocket:
            port: 6432
          initialDelaySeconds: 5
          periodSeconds: 10
        readinessProbe:
          exec:
            command:
            - sh
            - -c
            - "pg_isready -h 127.0.0.1 -p 6432"
          initialDelaySeconds: 3
          periodSeconds: 5
      volumes:
      - name: config
        configMap:
          name: pgbouncer-config
      - name: secrets
        secret:
          secretName: pgbouncer-credentials
---
apiVersion: v1
kind: Service
metadata:
  name: pgbouncer
  namespace: database
spec:
  selector:
    app: pgbouncer
  ports:
  - port: 6432
    targetPort: 6432
  type: ClusterIP
```

### ProxySQL 部署

```yaml
# 🟡 中风险：部署 ProxySQL（MySQL 读写分离）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: proxysql
  namespace: database
spec:
  serviceName: proxysql-headless
  replicas: 2
  selector:
    matchLabels:
      app: proxysql
  template:
    metadata:
      labels:
        app: proxysql
    spec:
      containers:
      - name: proxysql
        image: proxysql/proxysql:2.6.5
        ports:
        - containerPort: 6033
          name: mysql
        - containerPort: 6032
          name: admin
        - containerPort: 6070
          name: stats
        resources:
          requests:
            cpu: "1"
            memory: 1Gi
          limits:
            cpu: "2"
            memory: 2Gi
        volumeMounts:
        - name: config
          mountPath: /etc/proxysql.cnf
          subPath: proxysql.cnf
        - name: data
          mountPath: /var/lib/proxysql
        livenessProbe:
          tcpSocket:
            port: 6033
          initialDelaySeconds: 10
          periodSeconds: 15
      volumes:
      - name: config
        configMap:
          name: proxysql-config
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3-encrypted
      resources:
        requests:
          storage: 5Gi
```

### ProxySQL 读写分离配置

```sql
-- 🟡 中风险：配置 ProxySQL 读写分离规则
-- 通过 admin 端口连接
-- mysql -h proxysql-0.proxysql-headless.database.svc -P 6032 -u admin -padmin

-- 添加 MySQL 后端服务器
INSERT INTO mysql_servers (hostgroup_id, hostname, port, max_connections)
VALUES
  (10, 'mysql-primary.database.svc', 3306, 100),   -- 写组
  (20, 'mysql-replica-0.database.svc', 3306, 200), -- 读组
  (20, 'mysql-replica-1.database.svc', 3306, 200); -- 读组

-- 配置查询路由规则
INSERT INTO mysql_query_rules (rule_id, active, match_pattern, destination_hostgroup, apply)
VALUES
  (1, 1, '^SELECT.*FOR UPDATE$', 10, 1),  -- SELECT FOR UPDATE → 写组
  (2, 1, '^SELECT', 20, 1);                -- 其他 SELECT → 读组

-- 配置用户
INSERT INTO mysql_users (username, password, default_hostgroup, max_connections)
VALUES ('app_user', 'hashed_password', 10, 500);

-- 加载配置
LOAD MYSQL SERVERS TO RUNTIME;
LOAD MYSQL QUERY RULES TO RUNTIME;
LOAD MYSQL USERS TO RUNTIME;
SAVE MYSQL SERVERS TO DISK;
SAVE MYSQL QUERY RULES TO DISK;
SAVE MYSQL USERS TO DISK;
```

## 运维操作

### 连接池参数调优

**PgBouncer 关键参数：**

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| max_client_conn | 1000-5000 | 最大客户端连接数 |
| default_pool_size | 20-50 | 每用户每数据库的默认池大小 |
| min_pool_size | 5-10 | 最小保持连接数 |
| reserve_pool_size | 5 | 突发流量预留连接 |
| reserve_pool_timeout | 3s | 等待多久后使用预留连接 |
| server_idle_timeout | 300s | 空闲连接回收时间 |
| server_lifetime | 3600s | 连接最大存活时间（防止内存泄漏） |
| query_timeout | 30s | 单条查询超时 |
| client_idle_timeout | 600s | 客户端空闲超时（防连接泄漏） |

**池大小计算公式：**
```
pool_size = (CPU核心数 × 2) + 有效磁盘数
示例：8核 SSD → pool_size = 17 ≈ 20
总后端连接 = replicas × default_pool_size × 数据库数
```

### 监控

```bash
# 🟢 低风险：PgBouncer 状态查看
kubectl exec -n database deploy/pgbouncer -- \
  psql -h 127.0.0.1 -p 6432 -U pgbouncer pgbouncer -c "SHOW POOLS;"

# 🟢 低风险：PgBouncer 统计信息
kubectl exec -n database deploy/pgbouncer -- \
  psql -h 127.0.0.1 -p 6432 -U pgbouncer pgbouncer -c "SHOW STATS;"

# 🟢 低风险：ProxySQL 连接状态
mysql -h proxysql-0.proxysql-headless.database.svc -P 6032 -u admin -padmin \
  -e "SELECT hostgroup, srv_host, status, ConnUsed, ConnFree, Queries FROM stats.stats_mysql_connection_pool;"

# 🟢 低风险：ProxySQL 查询统计
mysql -h proxysql-0.proxysql-headless.database.svc -P 6032 -u admin -padmin \
  -e "SELECT digest_text, count_star, sum_time_us/1000 as total_ms FROM stats.stats_mysql_query_digest ORDER BY sum_time_us DESC LIMIT 10;"
```

## 故障排查

### 连接泄漏

**现象**：`SHOW POOLS` 显示 `cl_active` 持续增长不释放。

```bash
# 🟢 低风险：查看 PgBouncer 客户端连接详情
kubectl exec -n database deploy/pgbouncer -- \
  psql -h 127.0.0.1 -p 6432 -U pgbouncer pgbouncer -c "SHOW CLIENTS;"

# 🟢 低风险：查看长时间空闲的客户端连接
kubectl exec -n database deploy/pgbouncer -- \
  psql -h 127.0.0.1 -p 6432 -U pgbouncer pgbouncer \
  -c "SELECT * FROM SHOW CLIENTS WHERE state = 'active' AND now() - link_time > interval '5 minutes';"
```

**解决方案**：
1. 设置 `client_idle_timeout = 300` 自动断开空闲客户端
2. 应用层确保连接/事务正确关闭（使用连接池客户端库）
3. 设置 `server_lifetime` 定期轮换后端连接

### 池耗尽

**现象**：应用报 "no more connections allowed" 或查询排队超时。

```bash
# 🟢 低风险：检查池使用率
kubectl exec -n database deploy/pgbouncer -- \
  psql -h 127.0.0.1 -p 6432 -U pgbouncer pgbouncer -c "SHOW POOLS;"
# 关注 cl_waiting > 0 表示有客户端在排队

# 🟢 低风险：检查后端数据库连接数
kubectl exec -n database postgres-primary-0 -- \
  psql -U postgres -c "SELECT count(*) as total, state FROM pg_stat_activity GROUP BY state;"
```

**解决方案**：
1. 增大 `default_pool_size`（需确认数据库 `max_connections` 足够）
2. 增加 PgBouncer 副本数（注意总连接数 = 副本 × pool_size）
3. 排查慢查询占用连接时间过长
4. 应用层减少连接持有时间（避免在事务中做 HTTP 调用）

### ProxySQL 后端故障切换

```bash
# 🟢 低风险：检查后端服务器健康状态
mysql -h proxysql-0.proxysql-headless.database.svc -P 6032 -u admin -padmin \
  -e "SELECT * FROM monitor.mysql_server_connect_log ORDER BY time_start_us DESC LIMIT 10;"

# 🟡 中风险：手动将故障节点设为 OFFLINE
mysql -h proxysql-0.proxysql-headless.database.svc -P 6032 -u admin -padmin \
  -e "UPDATE mysql_servers SET status='OFFLINE_HARD' WHERE hostname='mysql-replica-1.database.svc'; LOAD MYSQL SERVERS TO RUNTIME;"
```

## 最佳实践

1. **池化模式选择**：PostgreSQL 优先使用 Transaction 模式；如需 PREPARE 语句，使用 Session 模式或应用层处理
2. **连接数规划**：总后端连接数 ≤ 数据库 `max_connections` × 0.8，预留 20% 给管理和紧急连接
3. **高可用**：PgBouncer 部署 3+ 副本，前置 Service 负载均衡；ProxySQL 使用 Cluster 模式同步配置
4. **健康检查**：配置 liveness/readiness probe，确保故障 Pod 及时摘除
5. **监控告警**：关注 `cl_waiting`（排队数）、`sv_active`（活跃后端连接）、等待时间 P99，接入 [[可观测性/]] 平台
6. **优雅重启**：使用 `SHUTDOWN WAIT_FOR_CLIENTS` 而非直接 kill，避免中断活跃事务
7. **TLS 配置**：客户端到连接池、连接池到数据库均启用 TLS，参考 [[数据库中间件/数据库/]] 中的安全配置
8. **与 Operator 配合**：如使用 CloudNativePG 或 Zalando Operator，连接池通常内置，参考 [[数据库中间件/Operator管理/]]

## Related

- [[数据库中间件/数据库/]]
- [[可观测性/]]
- [[数据库中间件/Operator管理/]]
- [[数据库中间件/新型数据库/03-newsql-cockroachdb-yugabytedb.md]]
- [[可靠性/备份恢复/]]
