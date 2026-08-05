---
title: MySQL on Kubernetes 生产指南
description: 面向在 Kubernetes 上运行 MySQL 的生产指南，覆盖高可用架构、MySQL Group Replication / Operator 选型、备份、监控告警、故障转移与慢查询治理。
summary: 面向 Kubernetes 上 MySQL 的生产指南，覆盖 HA、Group Replication/Operator、备份、监控、故障转移与慢查询治理。
category: database-middleware
tags:
- production
- best-practices
- playbook
- database-middleware
- mysql
- group-replication
- operator
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
- Kubernetes 上如何生产化运行 MySQL
- MySQL Group Replication on K8s 配置
- MySQL on K8s 备份与故障转移
- MySQL 慢查询与监控治理
trigger_keywords:
- MySQL
- Group Replication
- MySQL Operator
- InnoDB Cluster
- Percona XtraDB Cluster
- slow query
prerequisites:
- kubectl-basics
- mysql-basics
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


# MySQL on Kubernetes 生产指南

本指南面向需要在 Kubernetes 上以生产标准运行 MySQL 的 SRE 与数据库工程师，提供高可用架构、Group Replication / Operator 选型、备份、监控告警、故障转移与慢查询治理的完整操作路径。MySQL 作为广泛使用的关系型数据库，在 Kubernetes 上运行时需要特别关注一致性复制、自动故障切换、备份可恢复性与性能调优。与无状态应用不同，数据库的运维需要理解存储、网络、复制拓扑与 Operator 行为之间的复杂关系。本指南中的命令与配置可直接在已安装 `kubectl` 与 `helm` 的环境中执行，所有重大变更应先在测试集群验证，并遵循 [[32-发布/package/2026-07-02_18-40/corpus/core/domain-16-database-middleware/03-production-readiness-operations-guide|生产就绪运维框架]] 中的变更管理要求。

## 1. 适用场景与范围

本指南适用于以下场景：

- 在 Kubernetes 上部署生产级 MySQL 集群。
- 需要实现自动故障转移、读写分离、备份恢复。
- 需要建立监控告警、慢查询分析与性能基线。
- 排查 MySQL Pod 异常、复制延迟、备份失败、连接数耗尽等问题。
- 需要理解 MySQL Group Replication、InnoDB Cluster 与 Percona XtraDB Cluster 的适用场景。

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必需工具
kubectl version
helm version

# 推荐方案
# MySQL Operator for Kubernetes（Oracle）：https://dev.mysql.com/doc/mysql-operator/en/
# Percona Operator for MySQL：
# MySQL Group Replication / InnoDB Cluster
# Percona XtraDB Cluster (PXC)
```
建议为数据库节点池配置：
- 高 IOPS SSD（如 AWS io2、GCP Hyperdisk、Azure Premium SSD v2）。
- 充足的 CPU/内存，避免与突发负载共享节点。
- Pod 反亲和性，确保副本分布在不同节点与可用区。
- 专用网络策略，限制仅允许应用与监控组件访问数据库端口。

## 3. 核心概念与架构

### 3.1 高可用拓扑

```
[Primary]  <-- Group Replication -->  [Secondary 1]
       |                                   |
       +--------------------------> [Secondary 2]
```

- **MySQL Group Replication（MGR）**：基于 Paxos 的同步复制，自动故障检测与切换。
- **InnoDB Cluster**：在 MGR 之上提供 MySQL Shell 管理与 Router 读写分离。
- **Percona XtraDB Cluster**：基于 Galera 的同步多主复制，适合写扩展场景。

生产环境建议至少部署 3 个节点，确保单点故障时仍能维持 quorum 与自动切换。

### 3.2 Operator 选型

| 方案 | 特点 | 适用场景 |
|---|---|---|
| **MySQL Operator for Kubernetes** | Oracle 官方、集成 InnoDB Cluster | 新建集群、需要官方支持 |
| **Percona Operator for MySQL** | 成熟、支持 PXC 与 Group Replication | 已有 Percona 生态、需要多主 |
| **Bitnami MySQL Helm Chart** | 简单、适合单实例或主从 | 开发测试、非核心生产 |

生产建议：核心生产使用 MySQL Operator for Kubernetes 或 Percona Operator；避免使用无 Operator 的裸 StatefulSet 管理集群生命周期。

## 4. 标准操作流程

### 4.1 使用 MySQL Operator 部署 InnoDB Cluster

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Operator
helm repo add mysql-operator https://mysql.github.io/mysql-operator/
helm install mysql-operator mysql-operator/mysql-operator \
  --namespace database --create-namespace

# 创建 InnoDB Cluster
cat <<EOF | kubectl apply -f -
apiVersion: mysql.oracle.com/v2
kind: InnoDBCluster
metadata:
  name: prod-mysql
  namespace: database
spec:
  secretName: prod-mysql-secret
  instances: 3
  version: "8.0"
  tlsUseSelfSigned: true
  datadirVolumeClaimTemplate:
    storageClassName: premium-rwo
    resources:
      requests:
        storage: 100Gi
  podSpec:
    resources:
      requests:
        cpu: "2"
        memory: 4Gi
      limits:
        cpu: "4"
        memory: 8Gi
    affinity:
      podAntiAffinity:
        preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 100
          podAffinityTerm:
            labelSelector:
              matchLabels:
                app.kubernetes.io/name: prod-mysql
            topologyKey: topology.kubernetes.io/zone
EOF
```
创建 Secret：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create secret generic prod-mysql-secret \
  --namespace database \
  --from-literal=rootUser=root \
  --from-literal=rootHost=% \
  --from-literal=rootPassword='<STRONG_PASSWORD>'
```
### 4.2 使用 Percona Operator 部署 PXC

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Operator
helm repo add percona https://percona.github.io/percona-helm-charts/
helm install pxc-operator percona/pxc-operator -n database --create-namespace

# 部署集群（参考官方样例）
kubectl apply -f https://raw.githubusercontent.com/percona/percona-xtradb-cluster-operator/main/deploy/cr.yaml -n database
```
### 4.3 备份

MySQL Operator 备份示例：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: mysql.oracle.com/v2
kind: MySQLBackup
metadata:
  name: prod-mysql-backup
  namespace: database
spec:
  clusterName: prod-mysql
  backupProfileName: default
  backupSchedules:
  - name: daily
    schedule: "0 2 * * *"
    enabled: true
EOF
```
使用 Percona XtraBackup 物理备份：

```bash
# 在 Pod 内执行
xtrabackup --backup --target-dir=/backup/$(date +%F) \
  --user=backup --password='<PASSWORD>'

# 准备恢复
xtrabackup --prepare --target-dir=/backup/2026-07-01
```

建议将备份文件上传到对象存储，并定期执行恢复演练。备份策略应包括：每日全量备份、binlog 持续归档、定期恢复演练。

### 4.4 监控

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启用 MySQL Exporter
helm install mysql-exporter prometheus-community/prometheus-mysql-exporter \
  -n monitoring \
  --set mysql.user=exporter \
  --set mysql.pass=<PASSWORD> \
  --set mysql.host=prod-mysql.database.svc.cluster.local \
  --set mysql.port=3306
```
关键指标：
- `mysql_up`：实例存活。
- `mysql_global_status_threads_connected` / `max_connections`：连接数使用率。
- `mysql_global_status_innodb_row_lock_waits`：锁等待。
- `mysql_slave_lag_seconds`：复制延迟（MGR 可通过 `performance_schema` 查询）。

### 4.5 故障转移

MySQL Operator for Kubernetes 自动处理 MGR 主节点故障。手动切换：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入 Operator 工具 Pod 或 MySQL Shell
kubectl exec -it prod-mysql-0 -n database -- mysqlsh \
  --uri root@prod-mysql-0.prod-mysql.database.svc.cluster.local:3306 \
  --js -e "dba.getCluster().setPrimaryInstance('prod-mysql-1.prod-mysql.database.svc.cluster.local:3306')"
```
### 4.6 慢查询治理

```sql
-- 启用慢查询日志
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 1;
SET GLOBAL log_output = 'TABLE';  -- 或 FILE

-- 查看慢查询
SELECT * FROM mysql.slow_log ORDER BY start_time DESC LIMIT 20;

-- 启用 performance_schema 中的 statements 表
UPDATE performance_schema.setup_consumers SET ENABLED='YES' WHERE NAME LIKE '%statements%';
```

建议结合 pt-query-digest 或 PMM 进行慢查询聚合分析。慢查询治理应作为持续优化过程，定期 review TOP N 慢查询并优化索引或 SQL。

## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| 集群状态 | `kubectl get innodbcluster -n database` | ONLINE，3 成员 |
| 主节点 | `kubectl get innodbcluster prod-mysql -n database -o jsonpath='{.status.primary}'` | 存在且 Ready |
| 复制状态 | MySQL 内 `SELECT * FROM performance_schema.replication_group_members;` | 所有成员 ONLINE |
| 连接数 | MySQL 内 `SHOW STATUS LIKE 'Threads_connected';` | < max_connections * 80% |
| 备份状态 | `kubectl get mysqlbackup -n database` | Completed |
| 慢查询 | MySQL 内 `SELECT COUNT(*) FROM mysql.slow_log WHERE start_time > NOW() - INTERVAL 1 HOUR;` | 无突增 |

## 6. 常见故障与 remediation

| 现象 | 根因 | 处理命令/步骤 |
|---|---|---|
| Pod 持续 CrashLoopBackOff | 数据目录不一致、Secret 错误、资源不足 | 查看日志；检查 Secret 与 PVC 状态 |
| Group Replication 分裂 | 网络分区、节点间时钟不同步 | 检查网络连通性；配置 NTP；必要时重建集群 |
| 复制延迟高 | 大事务、I/O 瓶颈、网络抖动 | 拆分大事务；优化存储 I/O；检查网络带宽 |
| 连接数耗尽 | 应用未使用连接池、连接泄漏 | 部署 ProxySQL/MySQL Router；检查应用生命周期 |
| 备份失败 | 对象存储凭证过期、锁等待超时 | 检查 Secret；调整备份窗口避开高峰 |
| 慢查询突增 | 索引缺失、统计信息过期 | 启用慢查询日志；执行 `ANALYZE TABLE`；优化索引 |
| 主节点切换后应用连不上 | DNS/Service 未指向新主节点 | 使用 MySQL Router/ProxySQL 自动路由 |

## 7. 风险与注意事项

1. **MGR 需要至少 3 个节点**：2 节点无法容忍故障，单节点无法形成 quorum。
2. **存储性能决定写入延迟**：生产环境必须使用高 IOPS 存储，避免使用普通磁盘。
3. **大版本升级需谨慎**：MySQL 8.0 升级需使用逻辑导出导入或滚动升级，需提前验证兼容性。
4. **Group Replication 对网络要求严格**：节点间高延迟或丢包会导致频繁视图变更。
5. **备份不等于可恢复**：每月执行恢复演练，验证全量备份与 binlog 恢复的 RTO/RPO。
6. **Secret 与密码轮换**：避免明文密码；使用 External Secrets Operator 或 Vault 动态注入。
7. **网络隔离**：通过 NetworkPolicy 限制仅允许应用访问数据库端口。
8. **资源竞争**：数据库 Pod 应配置 Guaranteed QoS，避免与突发工作负载共享节点。

## 8. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-16-database-middleware/03-production-readiness-operations-guide|生产运维域生产就绪运维指南]]
- [[domain-16-database-middleware/数据库/01-mysql-enterprise-database.md|MySQL 企业数据库]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-16-database-middleware/02-databases/07-mysql-group-replication-topology|MySQL Group Replication 拓扑]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-16-database-middleware/03-production-readiness-operations-guide|存储数据域生产就绪指南]]
- [[domain-09-reliability-engineering/README.md|可靠性工程域]]
- [[domain-05-security-compliance/README.md|安全合规域]]


<!-- risk-assessed -->
