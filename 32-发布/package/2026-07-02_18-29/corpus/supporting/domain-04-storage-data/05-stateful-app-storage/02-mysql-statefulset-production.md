---
title: MySQL StatefulSet 生产部署指南
description: 面向阿里云/专有云 K8s 的 MySQL StatefulSet 生产部署方案，覆盖主从架构、半同步复制、定时备份、故障切换与恢复演练。
summary: 面向阿里云/专有云 K8s 的 MySQL StatefulSet 生产部署方案，覆盖主从架构、半同步复制、定时备份、故障切换与恢复演练。
category: storage
tags:
- k8s
- statefulset
- mysql
- replication
- backup
- alicloud
- apsara-stack
- production
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 数据库管理员
- 运维工程师
estimated_read_time: 25min
intent_queries:
- MySQL StatefulSet 生产部署
- K8s 上 MySQL 主从复制与高可用
- 阿里云 K8s MySQL 备份与故障切换
trigger_keywords:
- MySQL
- StatefulSet
- 主从复制
- xtrabackup
- 故障切换
- 阿里云 MySQL
prerequisites:
- kubectl-basics
- statefulset-basics
- mysql-basics
- storage-basics
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




# MySQL StatefulSet 生产部署指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，系统讲解 MySQL 通过 StatefulSet 部署时的主从架构、备份策略、故障切换与日常运维。

## 目录

1. [架构选型](#架构选型)
2. [StatefulSet + Headless Service 部署](#statefulset--headless-service-部署)
3. [主从复制与半同步配置](#主从复制与半同步配置)
4. [定时备份策略](#定时备份策略)
5. [故障切换与恢复](#故障切换与恢复)
6. [阿里云/专有云存储对接](#阿里云专有云存储对接)
7. [监控告警](#监控告警)
8. [常见问题与排错](#常见问题与排错)
9. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 架构选型

### 1.1 部署模式对比

在 Kubernetes 上运行 MySQL 有多种方式，每种方式在可用性、运维复杂度、成本与数据一致性上各有权衡。下表总结了常见的生产部署模式：

| 模式 | 副本数 | 自动切换 | 数据一致性 | 运维复杂度 | 适用场景 |
|:---|:---:|:---:|:---:|:---:|:---|
| 单实例 StatefulSet | 1 | 否 | 依赖单盘 | 低 | 开发测试 |
| 主从异步复制 | 1 主 + N 从 | 需 Operator | 最终一致 | 中 | 读多写少 |
| 主从半同步复制 | 1 主 + 2 从 | 需 Operator | 至少一个从库收到 | 中 | 生产推荐 |
| MySQL Group Replication | 3+ | MGR 自带 | 多数派提交 | 高 | 高一致场景 |
| 阿里云 RDS / PolarDB | 托管 | 是 | 强一致 | 低 | 核心生产优先 |

对于阿里云/专有云 K8s 环境，核心业务强烈建议直接使用 **阿里云 RDS MySQL 或 PolarDB**。托管数据库在自动备份、故障切换、补丁管理和性能优化方面具有明显优势。若因合规要求、成本约束或特殊网络隔离需要自建 MySQL，则推荐采用 **StatefulSet + 半同步复制 + 定时物理备份** 的方案。

### 1.2 为什么 StatefulSet 适合 MySQL

MySQL 是典型的有状态应用，对网络标识和持久存储有严格要求。Deployment 无法保证 Pod 重建后使用相同的网络名称和持久卷，而 StatefulSet 提供以下关键能力：

- **稳定网络标识**：每个 Pod 拥有可预测的 DNS 名称，例如 `mysql-0.mysql-headless.production.svc.cluster.local`，便于主从复制配置。
- **稳定持久存储**：PVC 与 Pod 序号绑定，Pod 重建后自动挂载原 PVC，保证数据不丢失。
- **有序部署与扩缩容**：Pod 按序号 0,1,2 顺序启动，逆序终止，便于主库优先初始化和安全缩容。

---

## 2. StatefulSet + Headless Service 部署

### 2.1 创建命名空间与 Headless Service

在正式部署 MySQL 之前，需要先创建独立的命名空间和 Headless Service。Headless Service 的 `clusterIP: None` 设置使得每个 Pod 都能通过稳定的 DNS 名称被直接访问，这是主从复制配置的基础。

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
---
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
  namespace: production
  labels:
    app: mysql
spec:
  clusterIP: None
  selector:
    app: mysql
  ports:
    - port: 3306
      name: mysql
    - port: 33060
      name: mysqlx
```

### 2.2 MySQL StatefulSet 示例

以下是一个生产可用的 MySQL StatefulSet 配置示例。该示例使用了 ESSD 云盘作为存储后端，并通过 Pod 反亲和性确保副本尽可能分散在不同节点上，提升故障容忍能力。

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: production
spec:
  serviceName: mysql-headless
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values:
                        - mysql
                topologyKey: kubernetes.io/hostname
      containers:
        - name: mysql
          image: mysql:8.0
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
            - name: MYSQL_REPLICA_USER
              value: repl
            - name: MYSQL_REPLICA_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: replica-password
          ports:
            - containerPort: 3306
              name: mysql
          resources:
            requests:
              cpu: "2"
              memory: "4Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
            - name: conf
              mountPath: /etc/mysql/conf.d
          livenessProbe:
            exec:
              command:
                - mysqladmin
                - ping
                - -uroot
                - -p${MYSQL_ROOT_PASSWORD}
            initialDelaySeconds: 60
            periodSeconds: 10
          readinessProbe:
            exec:
              command:
                - mysql
                - -uroot
                - -p${MYSQL_ROOT_PASSWORD}
                - -e
                - SELECT 1
            initialDelaySeconds: 30
            periodSeconds: 5
      volumes:
        - name: conf
          configMap:
            name: mysql-config
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 500Gi
```

### 2.3 MySQL 配置 ConfigMap

MySQL 的配置通过 ConfigMap 挂载到 `/etc/mysql/conf.d`。生产环境需要开启 binlog、GTID 和半同步复制，以保证数据可靠性和故障切换能力。

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-config
  namespace: production
data:
  mysql.cnf: |
    [mysqld]
    server-id = ${POD_NAME#mysql-} + 1
    log_bin = mysql-bin
    binlog_format = ROW
    gtid_mode = ON
    enforce_gtid_consistency = ON
    relay_log = mysql-relay-bin
    read_only = OFF
    innodb_buffer_pool_size = 2G
    innodb_log_file_size = 512M
    max_connections = 500
    expire_logs_days = 7
    # 半同步复制
    plugin-load = "rpl_semi_sync_master=semisync_master.so;rpl_semi_sync_slave=semisync_slave.so"
    rpl_semi_sync_master_enabled = 1
    rpl_semi_sync_slave_enabled = 1
    rpl_semi_sync_master_timeout = 1000
```

需要注意的是，`server-id` 在实际环境中应通过 init 脚本根据 Pod 名称动态生成，避免所有 Pod 使用相同的 server-id 导致复制异常。

---

## 3. 主从复制与半同步配置

### 3.1 初始化主从关系

StatefulSet 启动后，每个 MySQL 实例都是独立的。需要手动或通过初始化脚本将 `mysql-0` 配置为主库，其他节点配置为从库。

在配置复制之前，先查看 Pod 的有序网络标识，确认所有 Pod 已正常启动：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 MySQL Pod 的有序网络标识
kubectl get pods -n production -l app=mysql -o wide
```
将 `mysql-0` 作为主库，在其上创建复制用户：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在主库创建复制用户
kubectl exec -it mysql-0 -n production -- mysql -uroot -p$ROOT_PWD -e "
CREATE USER IF NOT EXISTS 'repl'@'%' IDENTIFIED WITH mysql_native_password BY '$REPL_PWD';
GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';
FLUSH PRIVILEGES;
"
```
然后在从库上配置主从复制。以 `mysql-1` 为例：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在从库配置主从复制（以 mysql-1 为例）
kubectl exec -it mysql-1 -n production -- mysql -uroot -p$ROOT_PWD -e "
CHANGE MASTER TO
  MASTER_HOST='mysql-0.mysql-headless.production.svc.cluster.local',
  MASTER_PORT=3306,
  MASTER_USER='repl',
  MASTER_PASSWORD='$REPL_PWD',
  MASTER_AUTO_POSITION=1;
START SLAVE;
"
```
### 3.2 验证复制状态

复制配置完成后，需要在从库上验证复制线程状态。以下命令可以检查 IO 线程和 SQL 线程是否正常运行，以及从库延迟情况：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在从库查看 Slave_IO_Running 与 Slave_SQL_Running 是否为 Yes
kubectl exec -it mysql-1 -n production -- mysql -uroot -p$ROOT_PWD -e "SHOW SLAVE STATUS\G"
```
关键字段检查：

| 字段 | 期望值 | 说明 |
|:---|:---|:---|
| `Slave_IO_Running` | Yes | IO 线程运行中 |
| `Slave_SQL_Running` | Yes | SQL 线程运行中 |
| `Seconds_Behind_Master` | 0 或很小 | 从库延迟 |
| `Last_IO_Error` | 空 | IO 错误信息 |
| `Last_SQL_Error` | 空 | SQL 错误信息 |

### 3.3 半同步复制的意义

异步复制在主库提交事务后不会等待从库确认，存在数据丢失风险。半同步复制要求主库在返回客户端成功之前，至少收到一个从库的确认，从而在主库故障时减少数据丢失。配置半同步复制时需要注意：

- `rpl_semi_sync_master_timeout` 不宜设置过大，避免从库延迟导致主库事务提交阻塞。
- 建议同时部署监控告警，及时发现复制延迟或半同步退化为异步的情况。

---

## 4. 定时备份策略

### 4.1 物理备份 CronJob

生产环境推荐使用 Percona XtraBackup 进行物理热备份。物理备份相比逻辑备份速度更快、对业务影响更小，并且支持基于时间点的恢复。

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: mysql-xtrabackup
  namespace: production
spec:
  schedule: "0 2 * * *"
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: xtrabackup
              image: percona/percona-xtrabackup:8.0
              command:
                - /bin/sh
                - -c
                - |
                  DATE=$(date +%Y%m%d-%H%M%S)
                  BACKUP_DIR=/backup/mysql-${DATE}
                  mkdir -p ${BACKUP_DIR}
                  xtrabackup --backup --host=mysql-0.mysql-headless.production.svc.cluster.local \
                    --user=root --password=$ROOT_PWD \
                    --target-dir=${BACKUP_DIR}
                  xtrabackup --prepare --target-dir=${BACKUP_DIR}
                  tar czf ${BACKUP_DIR}.tar.gz -C /backup mysql-${DATE}
                  ossutil cp ${BACKUP_DIR}.tar.gz oss://db-backup-bucket/mysql/production/
                  rm -rf ${BACKUP_DIR} ${BACKUP_DIR}.tar.gz
              env:
                - name: ROOT_PWD
                  valueFrom:
                    secretKeyRef:
                      name: mysql-secret
                      key: root-password
                - name: OSS_ENDPOINT
                  value: "oss-cn-hangzhou-internal.aliyuncs.com"
              volumeMounts:
                - name: backup-tmp
                  mountPath: /backup
          restartPolicy: OnFailure
          volumes:
            - name: backup-tmp
              emptyDir:
                sizeLimit: 200Gi
```

### 4.2 binlog 归档

除了每日全量备份，还需要持续归档 binlog，以支持按时间点恢复。binlog 归档频率可以根据业务写入量调整，通常建议每小时或每完成一个 binlog 文件后归档。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 每日归档 binlog 到 OSS，用于按时间点恢复
kubectl exec -it mysql-0 -n production -- mysql -uroot -p$ROOT_PWD -e "FLUSH BINARY LOGS"
ossutil cp -r /var/lib/mysql/mysql-bin.* oss://db-backup-bucket/mysql/binlog/
```
### 4.3 备份策略建议

| 备份类型 | 频率 | 保留周期 | 存储位置 | 用途 |
|:---|:---:|:---:|:---|:---|
| 全量物理备份 | 每日 | 30 天 | OSS 异地 | 灾难恢复 |
| 增量 binlog | 每小时 | 7 天 | OSS 异地 | 按时间点恢复 |
| 关键变更前 | 按需 | 长期 | OSS 冷存 | 变更回滚 |

---

## 5. 故障切换与恢复

### 5.1 主库不可用时的切换

当 `mysql-0` 发生故障且无法快速恢复时，可以将 `mysql-1` 提升为新的主库。切换前需要确认从库数据已基本同步，避免数据丢失。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 mysql-1 数据已同步完成
kubectl exec -it mysql-1 -n production -- mysql -uroot -p$ROOT_PWD -e "SHOW SLAVE STATUS\G" | grep Seconds_Behind_Master

# 2. 停止 mysql-1 的复制并提升为主库
kubectl exec -it mysql-1 -n production -- mysql -uroot -p$ROOT_PWD -e "STOP SLAVE; RESET SLAVE ALL; SET GLOBAL read_only=OFF;"

# 3. 修改应用连接地址指向 mysql-1
kubectl patch service mysql-write -n production -p '{"spec":{"selector":{"statefulset.kubernetes.io/pod-name":"mysql-1"}}}'
```
### 5.2 自动化故障切换

手动切换在大规模生产环境中容易出错，建议结合 MySQL Operator（如 Oracle MySQL Operator 或 Percona XtraDB Cluster Operator）实现自动故障检测与切换。自动切换方案需要满足以下条件：

- 可靠的 leader 选举机制，推荐使用 etcd 或 Consul。
- 读写分离的 Service 能够自动更新 selector 指向新主库。
- 完善的监控告警，及时通知运维人员切换事件。

### 5.3 基于 XtraBackup 的恢复

```bash
# 1. 从 OSS 下载备份
ossutil cp oss://db-backup-bucket/mysql/production/mysql-20260629-020000.tar.gz /restore/

# 2. 解压到新的空 PVC
tar xzf /restore/mysql-20260629-020000.tar.gz -C /var/lib/mysql

# 3. 执行 prepare
xtrabackup --prepare --target-dir=/var/lib/mysql/mysql-20260629-020000

# 4. 启动 MySQL 并重新配置复制
```

---

## 6. 阿里云/专有云存储对接

### 6.1 推荐 StorageClass

| 场景 | StorageClass | 性能等级 | 说明 |
|:---|:---|:---|:---|
| MySQL 主库 | alicloud-disk-essd | PL2/PL3 | 低延迟、高 IOPS |
| MySQL 从库 | alicloud-disk-essd | PL1/PL2 | 可稍低于主库 |
| 备份中转 | alicloud-disk-ssd | - | 仅临时存储 |

### 6.2 专有云 ASO 注意事项

在专有云环境中部署 MySQL 时，需要特别关注以下几点：

- 在 ASO 控制台确认 ESSD 库存充足，避免因库存不足导致 PVC 无法绑定。
- CSI 插件镜像需同步到专有云镜像仓库，并验证镜像版本与 K8s 版本兼容。
- 备份目标优先使用专有云 OSS 内网 Endpoint，降低公网流量成本。
- 如需满足合规要求，可在 StorageClass 中开启云盘加密。

---

## 7. 监控告警

### 7.1 PrometheusRule 示例

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: mysql-alerts
  namespace: monitoring
spec:
  groups:
    - name: mysql.rules
      rules:
        - alert: MySQLReplicationLagHigh
          expr: |
            mysql_slave_lag_seconds > 10
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "MySQL 从库延迟过高"
            description: "从库 {{ $labels.instance }} 延迟 {{ $value }} 秒"
        - alert: MySQLSlaveNotRunning
          expr: |
            mysql_slave_status_slave_io_running == 0 or
            mysql_slave_status_slave_sql_running == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "MySQL 复制线程停止"
        - alert: MySQLBackupFailed
          expr: |
            time() - mysql_backup_last_success_timestamp > 86400
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "MySQL 备份超过 24 小时未成功"
```

### 7.2 监控维度建议

| 维度 | 关键指标 | 告警阈值 |
|:---|:---|:---:|
| 复制 | Seconds_Behind_Master | > 10s |
| 复制 | Slave_IO/SQL_Running | != Yes |
| 性能 | Threads_running | > 100 |
| 存储 | 磁盘使用率 | > 85% |
| 备份 | 上次成功时间 | > 24h |

---

## 8. 常见问题与排错

### 8.1 Pod 启动失败

可能原因包括：
- Secret 中密码未正确配置
- PVC 未绑定，StorageClass 不存在或库存不足
- MySQL 配置文件语法错误

排查命令：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod mysql-0 -n production
kubectl logs mysql-0 -n production --previous
```
### 8.2 复制中断

常见原因包括：
- 主库 binlog 被清理，从库无法继续读取
- 主从 server-id 冲突
- 网络分区导致连接中断

处理方法：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看具体错误
kubectl exec -it mysql-1 -n production -- mysql -uroot -p$ROOT_PWD -e "SHOW SLAVE STATUS\G" | grep -E "Last_.*_Error"

# 若 GTID 一致，可尝试跳过错误事务后重新同步
kubectl exec -it mysql-1 -n production -- mysql -uroot -p$ROOT_PWD -e "STOP SLAVE; SET GTID_NEXT='xxx'; BEGIN; COMMIT; SET GTID_NEXT='AUTOMATIC'; START SLAVE;"
```
### 8.3 磁盘空间不足

MySQL 运行过程中 binlog、慢查询日志和临时文件可能快速增长。建议：
- 设置 `expire_logs_days` 自动清理 binlog。
- 监控磁盘使用率并配置扩容告警。
- 定期清理慢查询日志和审计日志。

---

## 9. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| 使用 Headless Service | 所有 MySQL Pod 可稳定寻址 | `kubectl get svc -n production mysql-headless` |
| PVC 使用 ESSD | 生产使用 ESSD PL2+ | `kubectl get pvc -n production` |
| Pod 反亲和性 | 副本跨节点部署 | `kubectl get pod -n production -l app=mysql -o wide` |
| 半同步复制开启 | 至少一个从库收到事务 | `SHOW VARIABLES LIKE '%semi%'` |
| 每日物理备份 | CronJob 成功执行 | `kubectl get cj -n production mysql-xtrabackup` |
| binlog 归档 | 保留 7 天以上 | 检查 OSS 备份桶 |
| 备份恢复演练 | 每季度一次 | 演练报告 |
| 监控告警覆盖 | 延迟、复制状态、备份 | PrometheusRule |

---

## 生产检查清单（完整版）

在阿里云或专有云环境中上线 MySQL StatefulSet 前，建议逐项完成以下检查：

1. 已为每个 MySQL 节点分配独立 ESSD 云盘，且 StorageClass 支持在线扩容。
2. Headless Service 与 StatefulSet 的 serviceName 一致，Pod 可通过稳定 DNS 访问。
3. 主从节点之间网络延迟低于 1ms，避免复制延迟过高。
4. 已配置 Pod 反亲和性，副本分布在不同节点与可用区。
5. 已创建单独的备份 Secret，root 与 repl 密码强度符合安全规范。
6. XtraBackup 与 binlog 归档任务已验证成功，备份文件可在 OSS 列出。
7. Orchestrator 或 MGR 的自动切换已演练通过，RTO 满足业务要求。
8. 已配置 PrometheusRule 监控 MySQL 主从延迟、连接数、慢查询。
9. 已设置资源 limit，并为 buffer pool 预留足够内存，避免 OOM。
10. 已文档化故障切换、PVC 扩容、密码重置三类常见操作。

## 阿里云/专有云环境差异

| 维度 | 阿里云 ACK | 专有云 ASO/天基 |
|:---|:---|:---|
| 存储申请 | ACK 控制台直接创建 StorageClass | ASO 控制台申请块存储后手动创建 SC |
| 备份目标 | 阿里云 OSS 标准/低频存储 | 专有云 OSS 或 NAS |
| 镜像仓库 | ACR 默认可用 | 需同步到专有云镜像仓库 |
| 监控接入 | 阿里云 Prometheus / ARMS | 自建 Prometheus/Grafana |
| 托管服务 | 可优先使用 RDS/PolarDB | 通常需自建 |

## 典型工单场景与处理

**场景**：用户反馈 MySQL 从库复制延迟持续上升。

处理步骤：
1. 使用 `SHOW REPLICA STATUS` 确认 Seconds_Behind_Source 数值。
2. 检查主库是否存在大事务或全表更新。
3. 使用 `kubectl top pod` 查看从库 CPU/IO 是否打满。
4. 检查 ESSD 云盘延迟与 IOPS 是否达到上限。
5. 如延迟无法快速恢复，考虑重建从库并重新加入复制。

## Related

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/05-stateful-app-storage/01-stateful-app-storage-patterns|有状态应用 Kubernetes 存储模式]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-04-storage-data/01-k8s-storage/03-storage-backup-disaster-recovery|存储备份与灾难恢复]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-09-reliability-engineering/01-backup-recovery/01-enterprise-backup-strategy|企业级备份策略]]

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/04-distributed-storage/01-velero-backup-recovery|Velero 阿里云专有云备份恢复实战]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/08-statefulset-troubleshooting|StatefulSet 故障诊断]]


<!-- risk-assessed -->
