---
title: 有状态服务运维
description: '# 有状态服务运维'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- prometheus
- redis
- mysql
- postgresql
- kafka
- elasticsearch
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 有状态服务运维 是什么
- 如何 有状态服务运维
trigger_keywords:
- 有状态服务运维
- dictionary
title_en: Services
---


# 有状态服务运维

## 概述

虽然 Kubernetes 最初为无状态应用设计，但近年来**有状态工作负载（Stateful Workloads）** 在 K8s 上的运行已日趋成熟。数据库（MySQL、PostgreSQL、MongoDB）、消息队列（Kafka、RabbitMQ）、缓存（Redis）和搜索引擎（Elasticsearch）等关键基础设施组件，越来越多地通过 **StatefulSet** 和 **Operator** 模式部署在 Kubernetes 中。2026 年的最佳实践要求 SRE 掌握有状态服务的高可用、备份恢复、存储性能和滚动升级策略。

## 核心概念/原理

### 1. StatefulSet 设计

**StatefulSet** 是 Kubernetes 专为有状态应用设计的控制器，提供：
- **稳定的网络标识**：Pod 名称按序编号（如 `mysql-0`, `mysql-1`），重建后保持不变
- **稳定的存储绑定**：每个 Pod 拥有独立的 PVC，即使 Pod 被重建也能挂载到原来的数据卷
- **有序的部署和扩缩容**：按顺序启动、停止和升级 Pod，避免数据不一致
- **Headless Service**：为每个 Pod 提供可直接访问的 DNS 记录（如 `mysql-0.mysql.default.svc.cluster.local`）

### 2. 持久化存储与数据一致性

有状态服务对存储的要求远高于无状态应用：
- **I/O 性能**：数据库需要高 IOPS 和低延迟，通常使用 SSD 或 NVMe 存储类
- **数据一致性**：同步复制（Synchronous Replication）确保主从节点数据一致，但会增加写入延迟
- **存储拓扑感知**：将主节点和副本节点调度到不同可用区，但又要保证网络低延迟

### 3. Operator 模式

手动管理有状态服务的生命周期极其复杂，**Operator** 应运而生：
- **自动化运维**：自动完成备份、恢复、故障转移、配置更新、滚动升级
- **领域知识内嵌**：将 DBA 的专业知识编码到 Controller 中
- **自愈能力**：自动检测主节点故障并提升从节点

主流有状态 Operator：
| 数据库 | Operator |
|--------|----------|
| PostgreSQL | CloudNativePG、Zalando Postgres Operator、CrunchyData |
| MySQL | Oracle MySQL Operator、Presslabs MySQL Operator |
| MongoDB | MongoDB Community/Enterprise Operator |
| Redis | Redis Operator、Spotahome Redis Operator |
| Kafka | Strimzi、Banzai Cloud Kafka Operator |
| Elasticsearch / OpenSearch | ECK (Elastic Cloud on Kubernetes) |
| Cassandra | K8ssandra |

### 4. 读写分离与分片

- **读写分离**：主节点处理写请求，副本节点处理读请求，通过 Service 或 Proxy（如 PgPool、ProxySQL）进行流量分发
- **分片（Sharding）**：将大数据集拆分到多个节点，如 MongoDB Sharded Cluster、Vitess for MySQL
- Kubernetes 上的分片管理通常需要额外的 Controller 或自定义调度器支持

## 关键机制或特性

### 高可用架构

| 架构 | 原理 | 适用数据库 |
|------|------|------------|
| **主从复制** | 一主多从，异步或半同步复制 | MySQL、PostgreSQL、Redis |
| **共享存储** | 多节点共享同一存储，故障时快速切换 | Oracle RAC（K8s 上较少） |
| **分布式共识** | 基于 Raft/Paxos 的自动 Leader 选举 | etcd、CockroachDB、TiDB |
| **分片集群** | 数据水平拆分，每个分片独立高可用 | MongoDB、Cassandra、Vitess |

### 备份策略

- **逻辑备份**：`pg_dump`、`mysqldump`、`mongodump`，适合小数据量和跨版本迁移
- **物理备份**：基于存储快照（VolumeSnapshot）或数据库原生工具（如 `pg_basebackup`、XtraBackup）
- **连续归档（WAL Archiving）**：PostgreSQL 和 MySQL 支持基于 WAL/Binlog 的 Point-in-Time Recovery（PITR）
- **跨集群复制**：使用 Operator 或数据库原生复制在异地建立灾备副本

### 滚动升级与兼容性

有状态服务的升级远比 Deployment 复杂：
- **小版本升级**：通常可以直接滚动升级（如 PostgreSQL 15.1 → 15.2）
- **大版本升级**：涉及数据格式变更，通常需要逻辑导出导入或使用专用升级工具（如 `pg_upgrade`）
- **升级窗口**：应在低峰期执行，并确保有完整的备份和回滚计划

## 使用场景

1. **PostgreSQL 高可用集群**：使用 CloudNativePG 部署 3 节点 Patroni 集群，自动故障转移 + WAL 归档备份
2. **Redis Cluster 分片管理**：通过 Redis Operator 部署 6 节点 Cluster（3 主 3 从），自动处理槽位分配和重平衡
3. **Kafka 日志平台**：使用 Strimzi Operator 管理 Kafka + ZooKeeper，自动进行 Topic 配置、滚动升级和证书轮换
4. **MongoDB 分片集群**：为大数据平台部署 MongoDB Sharded Cluster，通过 Operator 自动管理 Config Server 和 Mongos
5. **数据库跨云迁移**：利用 Vitess 的分片能力，将 MySQL 从 AWS RDS 平滑迁移到 Kubernetes 上的 Vitess 集群

## 最佳实践/注意事项

- **优先使用成熟的 Operator**：不要手工管理复杂数据库，选择社区活跃、文档完善的 Operator
- **存储类必须匹配数据库需求**：OLTP 数据库使用 high-iops SSD 存储类，分析型数据库可使用高吞吐型存储
- **为 StatefulSet 配置 Pod Disruption Budget（PDB）**：确保维护窗口期间不会同时中断过多的副本
- **监控延迟复制**：从库延迟（Replication Lag）是关键指标，过高会导致读操作返回旧数据
- **定期进行恢复演练**：备份不等于恢复，必须定期验证数据库从备份中恢复的速度和数据完整性
- **避免跨可用区写操作**：主从复制跨越多个 AZ 时，网络延迟会显著降低写入吞吐量
- **合理的资源限制**：数据库是资源密集型应用，必须准确设置 CPU/Memory Requests，避免被 OOM Killer 终止
- **配置独立的监控账号**：为 Prometheus/监控工具创建只读账号，避免使用高权限账号采集指标
- **数据卷扩容策略**：使用支持在线扩容的 StorageClass 和 CSI 驱动，避免停机扩容

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| StatefulSet Pod 启动失败 | PVC 绑定失败或存储类不存在 | `kubectl describe pod <sts-pod>` | 确认 StorageClass 存在且有可用容量 |
| 主从复制延迟过高 | 网络跨 AZ 延迟或从库 I/O 瓶颈 | 查看数据库 replication lag 指标 | 将副本调度到同 AZ 或升级存储 IOPS |
| Operator 状态异常 | CRD 版本不兼容或 RBAC 权限不足 | `kubectl logs -n <operator-ns> <operator-pod>` | 升级 Operator 或修正 ClusterRole |
| 数据库 failover 未触发 | Patroni/Operator 健康检查配置不当 | 查看 Operator CR status | 调整健康检查间隔和故障检测阈值 |
| 滚动升级导致数据损坏 | 大版本升级未使用升级工具 | `kubectl rollout status sts/<name>` | 大版本升级使用 `pg_upgrade` 等原生工具 |
| PVC 扩容失败 | CSI 驱动不支持在线扩容 | `kubectl describe pvc <pvc>` | 确认 StorageClass 的 `allowVolumeExpansion: true` |

## 生产检查清单

- [ ] 使用成熟的 Operator 管理有状态服务（CloudNativePG、Strimzi 等）
- [ ] StorageClass 使用高 IOPS SSD 类型
- [ ] StatefulSet 配置了 PDB（maxUnavailable: 1）
- [ ] 数据库复制延迟（Replication Lag）纳入告警
- [ ] 定期进行备份恢复演练
- [ ] 主从节点分布在不同可用区
- [ ] 数据库监控使用独立只读账号
- [ ] 数据卷使用支持在线扩容的 CSI 驱动
- [ ] 大版本升级有完整的备份和回滚计划

## 命令快速参考

```bash
# 查看 StatefulSet 状态
kubectl get sts -A
kubectl rollout status sts/<name> -n <namespace>

# 查看 PVC 绑定和容量
kubectl get pvc -n <namespace> -o wide

# CloudNativePG: 查看集群状态
kubectl get cluster -A
kubectl cnpg status <cluster-name> -n <namespace>

# Strimzi Kafka: 查看集群状态
kubectl get kafka -A
kubectl get kafkatopic -n <namespace>

# 手动触发 PostgreSQL 故障转移
kubectl cnpg promote <cluster-name> <target-pod> -n <namespace>

# 检查 PVC 扩容支持
kubectl get sc -o custom-columns=NAME:.metadata.name,EXPAND:.allowVolumeExpansion
```

## 交叉引用

- [CloudNativePG Documentation](https://cloudnative-pg.io/documentation/)
- [Strimzi Kafka Operator](https://strimzi.io/docs/operators/latest/)
- [Kubernetes StatefulSet Basics](https://kubernetes.io/docs/tutorials/stateful-application/basic-stateful-set/)
- 相关主题：[StatefulSets](../workloads/statefulsets.md) · [Persistent Volumes](../storage/persistent-volumes.md) · [备份与灾难恢复](backup-disaster-recovery.md) · [Operator Pattern](../platform-engineering/operator-pattern.md)
