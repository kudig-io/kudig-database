---
title: CloudNativePG (entities)
description: '## 概述'
summary: 'CloudNativePG 是 Kubernetes 上的 PostgreSQL Operator，提供完整的数据库生命周期管理。它原生支持 PostgreSQL 流复制、自动故障转移、备份恢复和监控集成。'
category: entities
tags:
- k8s
- cncf
- database
- cloudnativepg
- etcd
- prometheus
- grafana
- postgresql
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CloudNativePG 是什么
- 如何 CloudNativePG
trigger_keywords:
- CloudNativePG
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CloudNativePG

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

CloudNativePG 是由 EnterpriseDB（EDB）开发并开源的 Kubernetes 原生 PostgreSQL Operator，2022 年加入 CNCF Sandbox。它提供完整的 PostgreSQL 数据库生命周期管理，原生支持 PostgreSQL 流复制（Streaming Replication）、自动故障转移（Failover）、备份恢复（PITR）和监控集成。CloudNativePG 的核心理念是"无需外部依赖"——不依赖 etcd、Consul 或 Patroni 等第三方组件，完全利用 Kubernetes 原语实现 PostgreSQL 高可用。

## 核心特性

- **原生 HA**: 基于 PostgreSQL 流复制和 Kubernetes Lease 实现自动故障转移
- **无外部依赖**: 不需要 etcd、Consul 或 Patroni，纯 K8s 原生
- **声明式管理**: 通过 Cluster CRD 声明式管理 PostgreSQL 集群
- **备份与 PITR**: 基于 Barman 的连续 WAL 归档和时间点恢复
- **连接池**: 内置 PgBouncer 连接池管理
- **安全**: TLS 加密、证书自动轮换、SASL/SCRAM 认证

## 架构

CloudNativePG 采用主从（Primary-Replica）架构。Operator（Deployment）监听 Cluster CRD，管理 PostgreSQL 实例。每个 Cluster 创建一个由 Pod 组成的 StatefulSet，其中一个实例为 Primary（读写），其余为 Replica（只读）。Primary 选举基于 Kubernetes Lease 对象——当前持有 Lease 的 Pod 为 Primary。每个实例 Pod 包含 PostgreSQL 进程和 Instance Manager（sidecar 模式），后者负责实例配置、WAL 归档、健康检查。备份通过 Sidecar 执行 Barman 将 WAL 和基准备份推送到 S3/GCS/Azure。

## Kubernetes 集成

CloudNativePG 通过 Cluster CRD 定义 PostgreSQL 集群（副本数、存储、备份策略等）。Operator 管理 StatefulSet、Services（read-write、read-only）、ConfigMaps、Secrets。故障检测基于 Kubernetes Liveness Probe 和 Lease 续约。支持 PodAntiAffinity 实现跨可用区分布。通过 ServiceAccount 和 RBAC 管理权限。备份目标通过 Barman 的 S3/GCS/Azure 配置声明。

## 生产使用场景

1. **微服务数据库**: 每个微服务的 PostgreSQL 实例通过 CRD 声明式管理
2. **高可用数据库**: 3 节点 HA 集群，自动故障切换和恢复
3. **合规备份**: 连续 WAL 归档 + 定期全量备份，支持 PITR
4. **读写分离**: Primary 处理写请求，Replica 分担读请求

## 安装与配置

```bash
kubectl apply -f \
  https://raw.githubusercontent.com/cloudnative-pg/cloudnative-pg/release-1.24/cnpg-1.24.1.yaml
```

### Cluster CRD 配置示例

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: pg-cluster
  namespace: database
spec:
  instances: 3
  imageName: ghcr.io/cloudnative-pg/postgresql:16.2
  storage:
    size: 100Gi
    storageClass: fast-ssd
  walStorage:
    size: 20Gi
  postgresql:
    parameters:
      max_connections: "200"
      shared_buffers: "2GB"
      effective_cache_size: "6GB"
  backup:
    barmanObjectStore:
      destinationPath: s3://pg-backups/cluster1/
      s3Credentials:
        accessKeyId:
          name: s3-creds
          key: ACCESS_KEY
        secretAccessKey:
          name: s3-creds
          key: SECRET_KEY
    retentionPolicy: "30d"
  affinity:
    topologyKey: topology.kubernetes.io/zone
  resources:
    requests:
      memory: "4Gi"
      cpu: "2"
    limits:
      memory: "8Gi"
      cpu: "4"
```

## 运维操作

```bash
# 🟢 查看集群状态
kubectl get cluster -n database
kubectl describe cluster pg-cluster -n database

# 🟢 查看实例 Pod
kubectl get pods -n database -l cnpg.io/cluster=pg-cluster

# 🟢 查看复制状态
kubectl exec -n database pg-cluster-1 -- psql -U postgres -c "SELECT * FROM pg_stat_replication;"

# 🟡 手动触发备份
kubectl cnpg backup pg-cluster -n database

# 🟡 手动切换 Primary
kubectl cnpg promote pg-cluster-2 -n database

# 🟢 查看备份列表
kubectl cnpg status pg-cluster -n database

# 🟢 连接数据库
kubectl exec -it pg-cluster-1 -n database -- psql -U postgres

# 🟡 创建只读副本
kubectl patch cluster pg-cluster -n database \
  --type=merge -p '{"spec":{"instances":4}}'

# 🔴 时间点恢复 (PITR)
kubectl cnpg recovery pg-cluster --target-time "2024-01-15 10:30:00" -n database
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 集群未就绪 | PVC 未绑定 | `kubectl get pvc -n database` | 检查 StorageClass 和容量 |
| 复制延迟大 | 网络/负载问题 | `SELECT * FROM pg_stat_replication` | 检查网络和 WAL 生成速率 |
| 故障转移失败 | Lease 未释放 | 查看 Operator 日志 | 检查 RBAC 和 Lease 状态 |
| 备份失败 | S3 凭证问题 | 检查备份 Job 日志 | 更新 S3 凭证 Secret |
| 连接数耗尽 | 连接池未配置 | `SELECT count(*) FROM pg_stat_activity` | 配置 PgBouncer 连接池 |

## 生产案例

### 案例1: 自动故障转移

**场景**: Primary 节点宕机，需要自动切换  
**排查**: Operator 检测到 Lease 未续约，自动提升 Replica  
**效果**: 故障切换时间 < 30s，应用无感知  

### 案例2: PITR 恢复误删数据

**场景**: 开发人员误删生产表数据  
**方案**: 使用 PITR 恢复到删除前 1 分钟的状态  
**效果**: 15分钟内完成恢复，数据零丢失  

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **CloudNativePG** | 无外部依赖、EDB 支持 | 仅 PostgreSQL | 通用 PG 部署 |
| CrunchyData PGO | 功能全面 | 架构复杂 | 企业级 |
| Zalando Operator | 成熟稳定 | 架构较旧 | AWS 环境 |
| StackGres | 全功能 | 社区较小 | 多数据库 |

## 架构定位

在 CNCF 生态中，CloudNativePG 属于 **Database** 类别，是 PostgreSQL 在 Kubernetes 上的首选 Operator。它代表了“无需外部依赖”的云原生数据库管理理念。

## 检查清单

- [ ] 生产环境至少 3 个实例
- [ ] 配置跨可用区 PodAntiAffinity
- [ ] 配置连续 WAL 归档 + 定期全量备份
- [ ] 配置 PgBouncer 连接池
- [ ] 监控复制延迟和 WAL 生成速率
- [ ] 定期测试故障转移和 PITR 恢复
- [ ] 配置 TLS 加密和证书自动轮换

## 参考链接

- [[etcd]]
- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — Serverless Workflow
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference


<!-- risk-assessed -->
