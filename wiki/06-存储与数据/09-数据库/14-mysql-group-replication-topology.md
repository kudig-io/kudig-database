---
title: MySQL Group Replication on Kubernetes
description: 'InnoDB Cluster 配置、ProxySQL 路由、单主多主切换、在线 DDL、Percona XtraDB 备份恢复'
summary: 'InnoDB Cluster 配置、ProxySQL 路由、单主多主切换、在线 DDL、Percona XtraDB 备份恢复'
category: database-middleware
tags:
- database
- k8s
- mysql
- group-replication
- innodb-cluster
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DBA
- 平台工程师
estimated_read_time: 15min
intent_queries:
- MySQL Group Replication on Kubernetes 是什么
- 如何 MySQL Group Replication on Kubernetes
trigger_keywords:
- mysql
- group-replication
- innodb-cluster
- proxysql
- xtradb
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


# MySQL Group Replication on Kubernetes

## 1. 架构总览

```
┌───────────────────────────────────────────────────────────────────┐
│                         Application                               │
└──────────────────────────┬────────────────────────────────────────┘
                           │
                           ▼
┌───────────────────────────────────────────────────────────────────┐
│                  ProxySQL / MySQL Router                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │ 读写分离规则  │  │ 连接池管理   │  │ 健康检查     │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
└──────────────────────────┬────────────────────────────────────────┘
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
┌───────────────────────────────────────────────────────────────────┐
│                   MySQL Group Replication                         │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐                    │
│  │ Primary  │◄──►│Secondary │◄──►│Secondary │                    │
│  │ (读写)   │    │ (只读)   │    │ (只读)   │                    │
│  └──────────┘    └──────────┘    └──────────┘                    │
│       ▲               ▲               ▲                          │
│       └───────────────┼───────────────┘                          │
│           Paxos-based Consensus (XCom)                            │
└───────────────────────────────────────────────────────────────────┘
```

## 2. InnoDB Cluster 部署

### 2.1 MySQL 实例 StatefulSet

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-gr
  namespace: database
spec:
  serviceName: mysql-gr-headless
  replicas: 3
  selector:
    matchLabels:
      app: mysql-gr
  template:
    metadata:
      labels:
        app: mysql-gr
    spec:
      initContainers:
      - name: init-mysql
        image: mysql:8.0.36
        command:
        - bash
        - -c
        - |
          # 从 Pod 名称提取 server-id
          ORDINAL=${HOSTNAME##*-}
          echo "[mysqld]" > /mnt/conf.d/server-id.cnf
          echo "server-id=$((100 + ORDINAL))" >> /mnt/conf.d/server-id.cnf
          echo "report-host=${HOSTNAME}.mysql-gr-headless.database.svc.cluster.local" >> /mnt/conf.d/server-id.cnf
        volumeMounts:
        - name: conf
          mountPath: /mnt/conf.d
      containers:
      - name: mysql
        image: mysql:8.0.36
        ports:
        - containerPort: 3306
          name: mysql
        - containerPort: 33061
          name: grouprep
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-root-password
              key: password
        - name: MYSQL_ROOT_HOST
          value: "%"
        resources:
          requests:
            cpu: "4"
            memory: 16Gi
          limits:
            cpu: "8"
            memory: 32Gi
        volumeMounts:
        - name: conf
          mountPath: /etc/mysql/conf.d
        - name: data
          mountPath: /var/lib/mysql
        livenessProbe:
          exec:
            command: ["mysqladmin", "ping", "-uroot", "-p$(MYSQL_ROOT_PASSWORD)"]
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command:
            - bash
            - -c
            - |
              mysql -uroot -p$(MYSQL_ROOT_PASSWORD) -e "SELECT MEMBER_STATE FROM performance_schema.replication_group_members WHERE MEMBER_HOST LIKE '${HOSTNAME}%';" 2>/dev/null | grep -q ONLINE
          initialDelaySeconds: 45
          periodSeconds: 10
      volumes:
      - name: conf
        emptyDir: {}
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3-ssd
      resources:
        requests:
          storage: 200Gi
```

### 2.2 MySQL 配置文件

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-gr-config
  namespace: database
data:
  group-replication.cnf: |
    [mysqld]
    # Group Replication 基础配置
    gtid_mode=ON
    enforce_gtid_consistency=ON
    binlog_checksum=NONE
    log_bin=binlog
    binlog_format=ROW
    log_slave_updates=ON
    master_info_repository=TABLE
    relay_log_info_repository=TABLE
    transaction_write_set_extraction=XXHASH64

    # Group Replication 通信
    plugin_load_add='group_replication.so'
    group_replication_group_name="aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"
    group_replication_start_on_boot=OFF
    group_replication_local_address="mysql-gr-${ORDINAL}.mysql-gr-headless.database.svc.cluster.local:33061"
    group_replication_group_seeds="mysql-gr-0.mysql-gr-headless.database.svc.cluster.local:33061,mysql-gr-1.mysql-gr-headless.database.svc.cluster.local:33061,mysql-gr-2.mysql-gr-headless.database.svc.cluster.local:33061"
    group_replication_bootstrap_group=OFF

    # 单主模式
    group_replication_single_primary_mode=ON
    group_replication_enforce_update_everywhere_checks=OFF

    # 流量控制
    group_replication_flow_control_mode=QUOTA
    group_replication_flow_control_certifier_threshold=25000
    group_replication_flow_control_applier_threshold=25000

    # 压缩
    group_replication_compression_threshold=1000000

    # InnoDB 优化
    innodb_buffer_pool_size=12G
    innodb_buffer_pool_instances=8
    innodb_log_file_size=2G
    innodb_flush_log_at_trx_commit=1
    innodb_flush_method=O_DIRECT
    innodb_io_capacity=2000
    innodb_io_capacity_max=4000
```

### 2.3 集群初始化 Job

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: mysql-gr-init
  namespace: database
spec:
  backoffLimit: 5
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: init-cluster
        image: mysql:8.0.36
        command:
        - bash
        - -c
        - |
          set -e

          # 等待第一个 MySQL 实例就绪
          until mysql -h mysql-gr-0.mysql-gr-headless -uroot -p"${MYSQL_ROOT_PASSWORD}" -e "SELECT 1"; do
            echo "Waiting for mysql-gr-0..."
            sleep 5
          done

          # 在第一个节点引导集群
          mysql -h mysql-gr-0.mysql-gr-headless -uroot -p"${MYSQL_ROOT_PASSWORD}" <<'SQL'
          SET GLOBAL group_replication_bootstrap_group=ON;
          CREATE USER IF NOT EXISTS 'repl'@'%' IDENTIFIED BY 'repl_password';
          GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';
          GRANT CONNECTION_ADMIN ON *.* TO 'repl'@'%';
          GRANT GROUP_REPLICATION_STREAM ON *.* TO 'repl'@'%';
          FLUSH PRIVILEGES;
          CHANGE REPLICATION SOURCE TO SOURCE_USER='repl', SOURCE_PASSWORD='repl_password' FOR CHANNEL 'group_replication_recovery';
          START GROUP_REPLICATION;
          SET GLOBAL group_replication_bootstrap_group=OFF;
          SQL

          # 等待第一个节点 ONLINE
          sleep 10

          # 加入其余节点
          for i in 1 2; do
            until mysql -h mysql-gr-${i}.mysql-gr-headless -uroot -p"${MYSQL_ROOT_PASSWORD}" -e "SELECT 1"; do
              echo "Waiting for mysql-gr-${i}..."
              sleep 5
            done

            mysql -h mysql-gr-${i}.mysql-gr-headless -uroot -p"${MYSQL_ROOT_PASSWORD}" <<'SQL'
            SET GLOBAL group_replication_group_seeds='mysql-gr-0.mysql-gr-headless.database.svc.cluster.local:33061,mysql-gr-1.mysql-gr-headless.database.svc.cluster.local:33061,mysql-gr-2.mysql-gr-headless.database.svc.cluster.local:33061';
            CHANGE REPLICATION SOURCE TO SOURCE_USER='repl', SOURCE_PASSWORD='repl_password' FOR CHANNEL 'group_replication_recovery';
            START GROUP_REPLICATION;
            SQL
          done

          # 验证集群状态
          mysql -h mysql-gr-0.mysql-gr-headless -uroot -p"${MYSQL_ROOT_PASSWORD}" \
            -e "SELECT MEMBER_HOST, MEMBER_STATE, MEMBER_ROLE FROM performance_schema.replication_group_members;"
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-root-password
              key: password
```

## 3. ProxySQL 路由配置

### 3.1 ProxySQL 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: proxysql
  namespace: database
spec:
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
        image: proxysql/proxysql:2.6.3
        ports:
        - containerPort: 6033
          name: mysql
        - containerPort: 6032
          name: admin
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
      volumes:
      - name: config
        configMap:
          name: proxysql-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: proxysql-config
  namespace: database
data:
  proxysql.cnf: |
    datadir="/var/lib/proxysql"

    admin_variables=
    {
      admin_credentials="admin:admin_password"
      mysql_ifaces="0.0.0.0:6032"
    }

    mysql_variables=
    {
      threads=4
      max_connections=2048
      default_query_delay=0
      default_query_timeout=36000000
      have_compress=true
      poll_timeout=2000
      interfaces="0.0.0.0:6033"
      default_schema="information_schema"
      stacksize=1048576
      server_version="8.0.36"
      connect_timeout_server=3000
      monitor_username="monitor"
      monitor_password="monitor_password"
      monitor_history=600000
      monitor_connect_interval=60000
      monitor_ping_interval=10000
      monitor_read_only_interval=1500
      monitor_read_only_timeout=500
      set_query_lock_on_hostgroup=0
    }

    mysql_servers=
    (
      {
        address="mysql-gr-0.mysql-gr-headless.database.svc.cluster.local"
        port=3306
        hostgroup=10
        max_connections=100
        weight=1
      },
      {
        address="mysql-gr-1.mysql-gr-headless.database.svc.cluster.local"
        port=3306
        hostgroup=20
        max_connections=100
        weight=1
      },
      {
        address="mysql-gr-2.mysql-gr-headless.database.svc.cluster.local"
        port=3306
        hostgroup=20
        max_connections=100
        weight=1
      }
    )

    mysql_users=
    (
      {
        username="app_user"
        password="app_password"
        default_hostgroup=10
        max_connections=200
        default_schema="app_db"
        active=1
      }
    )

    mysql_query_rules=
    (
      {
        rule_id=100
        active=1
        match_digest="^SELECT .* FOR UPDATE$"
        destination_hostgroup=10
        apply=1
      },
      {
        rule_id=200
        active=1
        match_digest="^SELECT"
        destination_hostgroup=20
        apply=1
      }
    )

    mysql_replication_hostgroups=
    (
      {
        writer_hostgroup=10
        reader_hostgroup=20
        comment="MySQL Group Replication"
        check_type="read_only"
      }
    )
```

### 3.2 MySQL Router (备选方案)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mysql-router
  namespace: database
spec:
  replicas: 2
  selector:
    matchLabels:
      app: mysql-router
  template:
    metadata:
      labels:
        app: mysql-router
    spec:
      initContainers:
      - name: init-router
        image: mysql:8.0.36
        command:
        - bash
        - -c
        - |
          mysqlrouter --bootstrap root:${MYSQL_ROOT_PASSWORD}@mysql-gr-0.mysql-gr-headless:3306 \
            --directory /tmp/mysqlrouter \
            --conf-use-sockets \
            --account router_user \
            --account-create always
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-root-password
              key: password
        volumeMounts:
        - name: router-config
          mountPath: /tmp/mysqlrouter
      containers:
      - name: mysql-router
        image: mysql:8.0.36
        command: ["mysqlrouter"]
        args: ["--directory", "/tmp/mysqlrouter"]
        ports:
        - containerPort: 6446
          name: read-write
        - containerPort: 6447
          name: read-only
        volumeMounts:
        - name: router-config
          mountPath: /tmp/mysqlrouter
      volumes:
      - name: router-config
        emptyDir: {}
```

## 4. 单主/多主模式切换

### 4.1 模式对比

| 特性 | 单主模式 | 多主模式 |
|------|---------|---------|
| 写入节点 | 仅 Primary | 所有节点 |
| 冲突处理 | 无冲突 | 冲突检测 + 回滚 |
| 写入延迟 | 低 | 低（本地提交） |
| 读扩展 | 支持 | 支持 |
| DDL 操作 | Primary 执行 | 需停止 Group Replication |
| 适用场景 | 大多数 OLTP | 特定多写场景 |
| 推荐程度 | 生产首选 | 慎用 |

### 4.2 切换到多主模式

```sql
-- 在 Primary 节点执行
-- 1. 停止 Group Replication
STOP GROUP_REPLICATION;

-- 2. 修改配置
SET GLOBAL group_replication_single_primary_mode=OFF;
SET GLOBAL group_replication_enforce_update_everywhere_checks=ON;

-- 3. 重新引导（需在所有节点配置完成后）
SET GLOBAL group_replication_bootstrap_group=ON;
START GROUP_REPLICATION;
SET GLOBAL group_replication_bootstrap_group=OFF;

-- 4. 其余节点加入
-- 在每个 Secondary 节点执行
STOP GROUP_REPLICATION;
SET GLOBAL group_replication_single_primary_mode=OFF;
SET GLOBAL group_replication_enforce_update_everywhere_checks=ON;
START GROUP_REPLICATION;

-- 5. 验证
SELECT MEMBER_HOST, MEMBER_STATE, MEMBER_ROLE
FROM performance_schema.replication_group_members;
-- 所有节点应该显示 PRIMARY
```

### 4.3 切换回单主模式

```sql
-- 1. 在所有节点停止 Group Replication
STOP GROUP_REPLICATION;

-- 2. 修改配置
SET GLOBAL group_replication_single_primary_mode=ON;
SET GLOBAL group_replication_enforce_update_everywhere_checks=OFF;

-- 3. 在期望的 Primary 节点引导
SET GLOBAL group_replication_bootstrap_group=ON;
START GROUP_REPLICATION;
SET GLOBAL group_replication_bootstrap_group=OFF;

-- 4. 其余节点加入
START GROUP_REPLICATION;

-- 5. 验证
SELECT MEMBER_HOST, MEMBER_STATE, MEMBER_ROLE
FROM performance_schema.replication_group_members;
-- Primary 显示 PRIMARY，其余显示 SECONDARY
```

## 5. 在线 DDL 策略

### 5.1 MySQL 8.0 原生在线 DDL

```sql
-- Instant DDL（元数据变更，不锁表）
ALTER TABLE users ADD COLUMN nickname VARCHAR(64) DEFAULT '' AFTER name, ALGORITHM=INSTANT;

-- In-Place DDL（不拷贝表数据）
ALTER TABLE users ADD INDEX idx_email (email), ALGORITHM=INPLACE, LOCK=NONE;

-- Copy DDL（拷贝表数据，需避免）
ALTER TABLE users MODIFY COLUMN name VARCHAR(128), ALGORITHM=COPY;
```

### 5.2 pt-online-schema-change

```bash
# 对大表使用 pt-osc 避免长时间锁表
pt-online-schema-change \
  --alter "ADD COLUMN status TINYINT DEFAULT 0" \
  --host mysql-gr-0.mysql-gr-headless \
  --port 3306 \
  --user root \
  --password "${MYSQL_ROOT_PASSWORD}" \
  --database app_db \
  --table users \
  --execute \
  --chunk-size=1000 \
  --max-lag=1s \
  --check-interval=1 \
  --critical-load="Threads_running=100" \
  --progress=time,30
```

### 5.3 gh-ost（GitHub Online Schema Migration）

```bash
# gh-ost 不使用触发器，对 Group Replication 更友好
gh-ost \
  --host=mysql-gr-0.mysql-gr-headless \
  --port=3306 \
  --user=root \
  --password="${MYSQL_ROOT_PASSWORD}" \
  --database=app_db \
  --table=users \
  --alter="ADD COLUMN status TINYINT DEFAULT 0" \
  --allow-on-master \
  --allow-master-master \
  --chunk-size=1000 \
  --max-lag-millis=1500 \
  --serve-socket-file=/tmp/gh-ost.sock \
  --execute
```

### 5.4 DDL 策略选型

| 工具 | 锁类型 | 触发器 | 磁盘需求 | 适用场景 |
|------|-------|-------|---------|---------|
| Instant DDL | 无 | 无 | 无 | 元数据变更 |
| In-Place DDL | 短暂 MDL | 无 | 低 | 索引变更 |
| pt-osc | 短暂 MDL | 有 | 2x 表大小 | 通用大表 |
| gh-ost | 无 | 无 | 1.1x 表大小 | Group Replication 推荐 |

## 6. Percona XtraDB Cluster (PXC) 方案

### 6.1 PXC Operator 部署

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Percona Operator for MySQL
helm repo add percona https://percona.github.io/percona-helm-charts/
helm repo update

helm install pxcd percona/pxc-db \
  --namespace database --create-namespace \
  --set pxc.size=3 \
  --set pxc.image=percona/percona-xtradb-cluster:8.0.36 \
  --set pxc.resources.requests.cpu=4 \
  --set pxc.resources.requests.memory=16Gi \
  --set pxc.resources.limits.cpu=8 \
  --set pxc.resources.limits.memory=32Gi \
  --set pxc.volumeSpec.persistentVolumeClaim.storageClassName=gp3-ssd \
  --set pxc.volumeSpec.persistentVolumeClaim.resources.requests.storage=200Gi \
  --set haproxy.size=3 \
  --set haproxy.image=percona/haproxy:2.8.5
```
### 6.2 PXC 集群定义

```yaml
apiVersion: pxc.percona.com/v1-14-0
kind: PerconaXtraDBCluster
metadata:
  name: prod-pxc
  namespace: database
spec:
  crVersion: 1.14.0
  secretsName: prod-pxc-secrets
  allowUnsafeConfigurations: false
  pxc:
    size: 3
    image: percona/percona-xtradb-cluster:8.0.36
    resources:
      requests:
        cpu: "4"
        memory: 16Gi
      limits:
        cpu: "8"
        memory: 32Gi
    volumeSpec:
      persistentVolumeClaim:
        storageClassName: gp3-ssd
        resources:
          requests:
            storage: 200Gi
    affinity:
      antiAffinityTopologyKey: kubernetes.io/hostname
    configuration: |
      [mysqld]
      innodb_buffer_pool_size=12G
      innodb_log_file_size=2G
      max_connections=2048
      wsrep_trx_fragment_size=1M
      wsrep_trx_fragment_unit=bytes
  haproxy:
    size: 3
    image: percona/haproxy:2.8.5
    resources:
      requests:
        cpu: "1"
        memory: 1Gi
      limits:
        cpu: "2"
        memory: 2Gi
    serviceSpec:
      type: LoadBalancer
      annotations:
        service.beta.kubernetes.io/aws-load-balancer-type: nlb
  pmm:
    enabled: true
    image: percona/pmm-client:2.41.0
    serverHost: pmm-server.monitoring.svc.cluster.local
  backup:
    image: percona/percona-xtradb-cluster-operator:1.14.0-pxc8.0-backup
    storages:
      s3-backup:
        type: s3
        s3:
          bucket: pxc-backups
          region: us-east-1
          credentialsSecret: s3-credentials
    schedule:
    - name: daily-backup
      schedule: "0 2 * * *"
      keep: 7
      storageName: s3-backup
```

## 7. 备份恢复

### 7.1 XtraBackup 全量备份

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动触发备份
kubectl apply -f - <<EOF
apiVersion: pxc.percona.com/v1-14-0
kind: PerconaXtraDBClusterBackup
metadata:
  name: manual-backup-$(date +%Y%m%d)
  namespace: database
spec:
  pxcCluster: prod-pxc
  storageName: s3-backup
EOF

# 查看备份状态
kubectl get pxc-backup -n database
```
### 7.2 恢复流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看可用备份
kubectl get pxc-backup -n database

# 执行恢复
kubectl apply -f - <<EOF
apiVersion: pxc.percona.com/v1-14-0
kind: PerconaXtraDBClusterRestore
metadata:
  name: restore-from-backup
  namespace: database
spec:
  pxcCluster: prod-pxc
  backupName: manual-backup-20260702
EOF

# 监控恢复进度
kubectl get pxc-restore -n database -w
kubectl logs -f -n database job/restore-job-manual-backup-20260702
```
## 8. 监控告警

### 8.1 关键指标

| 指标 | 含义 | 告警阈值 |
|------|------|---------|
| `wsrep_cluster_size` | 集群节点数 | < 3 |
| `wsrep_ready` | 节点就绪状态 | != 1 |
| `wsrep_local_recv_queue` | 接收队列积压 | > 100 |
| `wsrep_local_send_queue` | 发送队列积压 | > 100 |
| `wsrep_flow_control_paused` | 流控暂停时间比 | > 0.1 |
| `wsrep_cert_deps_distance` | 并行应用差距 | < 10 |
| `Threads_running` | 活跃线程数 | > 200 |
| `Innodb_row_lock_waits` | 行锁等待 | 持续增长 |

### 8.2 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: mysql-gr-alerts
  namespace: monitoring
spec:
  groups:
  - name: mysql-group-replication
    rules:
    - alert: MySQLGroupReplicationNodeDown
      expr: mysql_global_status_wsrep_cluster_size < 3
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "MySQL Group Replication 节点数不足"
    - alert: MySQLGroupReplicationNotReady
      expr: mysql_global_status_wsrep_ready != 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "MySQL Group Replication 节点未就绪"
    - alert: MySQLGroupReplicationFlowControl
      expr: mysql_global_status_wsrep_flow_control_paused > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "MySQL Group Replication 流控暂停时间过长"
    - alert: MySQLGroupReplicationLag
      expr: mysql_slave_status_seconds_behind_master > 30
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "MySQL 复制延迟超过 30 秒"
```

## 9. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 节点无法加入 | `SELECT * FROM performance_schema.replication_group_members` | 网络不通、认证失败 |
| 写入冲突 | `SHOW ENGINE INNODB STATUS` | 多主模式下并发写同一行 |
| 复制延迟 | `SHOW SLAVE STATUS` | 大事务、网络延迟 |
| 流控触发 | `SHOW STATUS LIKE 'wsrep_flow%'` | 应用速度差异过大 |
| ProxySQL 路由错误 | `SELECT * FROM stats_mysql_query_digest` | 查询规则配置不当 |
| XtraBackup 失败 | 检查备份 Pod 日志 | 磁盘空间不足、权限问题 |


<!-- risk-assessed -->
