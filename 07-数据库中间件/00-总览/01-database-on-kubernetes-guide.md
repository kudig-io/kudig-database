---
title: 数据库在 Kubernetes 上的运行指南
summary: 数据库在 Kubernetes 上的运行指南：在 Kubernetes 上运行有状态数据库服务，既有云原生弹性优势，也面临存储性能、网络延迟和状态管理等技术挑战。本文涵盖
  MySQL、Redis、PostgreSQL 在 K8s 上的部署模式与运维要点。
category: 数据库中间件
tags:
- domain-16
- 数据库
- StatefulSet
- MySQL
- Redis
- PostgreSQL
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 数据库在 Kubernetes 上的运行指南

## 概述

在 Kubernetes 上运行有状态数据库服务，既有云原生弹性优势，也面临存储性能、网络延迟和状态管理等技术挑战。本文涵盖 MySQL、Redis、PostgreSQL 在 K8s 上的部署模式与运维要点。

## 数据库在 K8s 上的挑战

### 存储性能

数据库对 IO 延迟和吞吐量极为敏感。容器化的存储抽象层可能引入额外延迟：
- **块存储**：适合单节点数据库，IOPS 可预测
- **网络存储**：NAS/SAN 适合共享读场景，写入延迟较高
- **本地存储**：性能最佳，但数据持久性依赖节点稳定性

### 网络延迟

Pod 网络虚拟化对数据库连接的影响：
- Service 负载均衡增加一次网络跳转
- Headless Service 直接暴露 Pod IP，减少一跳延迟
- 建议数据库 Service 使用 `clusterIP: None`（Headless）模式

### 状态管理

数据库状态包括：数据文件、事务日志、配置文件、运行时状态。K8s 通过 [[statefulset|StatefulSet]] + [[persistent-volume-claim]] 提供有状态管理能力，但仍需 Operator 处理复杂生命周期。

## MySQL on K8s

### 基础部署模式

```yaml
# StatefulSet + PVC + Headless Service
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql-headless
  replicas: 3
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 50Gi
```

### MySQL Operator

推荐使用 [MySQL Operator](https://github.com/mysql/mysql-operator) 或 [Oracle MySQL Operator](https://github.com/oracle/mysql-operator)：
- 自动化主从切换
- 自动备份调度
- 支持组复制（Group Replication）

### 关键配置

| 配置项 | 建议值 | 说明 |
|---|---|---|
| innodb_buffer_pool_size | 内存的 70% | 缓存热数据 |
| innodb_flush_log_at_trx_commit | 1（生产） | 保证持久性 |
| max_connections | 根据业务调整 | 避免连接耗尽 |

## Redis on K8s

### 部署模式对比

| 模式 | 适用场景 | 持久化 | 复杂度 |
|---|---|---|---|
| 单节点 | 开发测试、缓存场景 | RDB/AOF | 低 |
| Sentinel | 高可用，读写分离 | 可选 | 中 |
| Cluster | 大规模分片，横向扩展 | 可选 | 高 |

### Redis Cluster on K8s

使用 [Redis Operator](https://github.com/OT-CONTAINER-KIT/redis-operator) 管理：
- 6 节点（3 主 3 从）为最小生产配置
- 每个节点独立 PVC，确保数据隔离
- 使用 Headless Service + StatefulSet 保证节点身份稳定

### 持久化策略

- **RDB**：定时快照，恢复速度快，可能丢失最后一次快照后的数据
- **AOF**：日志追加，数据完整性高，文件体积大
- 生产建议：同时开启 RDB + AOF，`appendfsync everysec`

## PostgreSQL on K8s

### Patroni 高可用

[Patroni](https://github.com/zalando/patroni) 是 PostgreSQL 高可用的标准方案：
- 基于 DCS（etcd/ZooKeeper/K8s Endpoints）实现 Leader 选举
- 自动故障切换（failover）
- 与 K8s 原生集成良好

```yaml
# Patroni 典型架构
# Patroni Pod (PostgreSQL + patroni 进程) × 3
# → 共享 DCS (etcd) 进行 Leader 选举
# → pgBouncer 连接池提供统一入口
```

### pgBouncer 连接池

数据库连接是宝贵资源，pgBouncer 可有效管理连接风暴：
- **Session 模式**：事务结束后归还连接
- **Transaction 模式**：事务级连接复用，推荐配置
- 最大连接数根据 `max_connections` 和 Pod 数量计算

## 阿里云 ACK：RDS 外接 vs 自建数据库

| 维度 | RDS 外接 | 自建数据库（K8s 内） |
|---|---|---|
| 运维复杂度 | 低（托管） | 高（需自行备份、监控） |
| 网络延迟 | 跨 VPC 有延迟 | 同集群内，延迟低 |
| 成本 | 按量计费，较贵 | 节点成本，可控 |
| 弹性伸缩 | 手动或自动（受限） | 容器级弹性 |
| 数据安全 | 阿里云负责 | 需自行配置备份策略 |

**建议**：生产核心数据优先使用 RDS；开发测试、缓存层可考虑 K8s 自建。

## 远程顾问诊断要点

作为远程顾问，面对数据库问题时按以下流程排查：

1. **连接超时排查**
   - 检查 Service Endpoint：`kubectl get endpoints <db-service>`
   - 检查网络策略（NetworkPolicy）是否拦截
   - 检查 DNS 解析：`nslookup <db-service>.<namespace>.svc.cluster.local`

2. **慢查询排查**
   - 要求客户执行 `SHOW PROCESSLIST`（MySQL）或 `pg_stat_activity`（PostgreSQL）
   - 检查是否存在锁等待或长时间运行的查询
   - 建议开启慢查询日志，分析 Top 10 慢 SQL

3. **存储瓶颈排查**
   - 检查 PVC 使用率：`kubectl get pvc` + `df -h`
   - 检查 StorageClass 的 IOPS/吞吐量限制
   - 如达到存储上限，参考 [[06-存储/01-K8s存储/02-pvc-expansion-guide.md|pvc-expansion-guide]] 进行扩容

> 数据库问题往往涉及数据安全，远程顾问应谨慎建议操作，关键操作（如删除数据、切换主从）需客户书面确认。

## 相关链接

- [[19-故障诊断/04-高级排障/structural-05-workloads/03-statefulset-troubleshooting.md|statefulset-troubleshooting]] — StatefulSet 问题排查
- [[19-故障诊断/02-资源排障/14-pvc-storage-troubleshooting.md|pvc-storage-troubleshooting]] — PVC 与存储问题排查
- [[persistent-volume-claim]] — PVC 原理与配置
- [[07-数据库中间件/00-总览/01-database-on-kubernetes-guide.md|mysql-operator-guide]] — MySQL Operator 详细配置

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
