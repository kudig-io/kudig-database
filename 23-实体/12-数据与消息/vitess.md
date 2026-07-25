---
title: Vitess (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- vitess
- etcd
- prometheus
- grafana
- argocd
- mysql
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Vitess 是什么
- 如何 Vitess
trigger_keywords:
- Vitess
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（信息收集，无副作用）。



# Vitess

> **CNCF 状态**: Graduated | **类别**: Database | **主要语言**: Go

## 概述

Vitess 是一个用于 MySQL 水平扩展的数据库集群系统和分片中间件，最初由 YouTube 开发并于 2018 年作为 CNCF 孵化项目加入，2020 年正式毕业（Graduated）。它将 MySQL 的功能与 NoSQL 数据库的可扩展性相结合，为大规模 MySQL 部署提供连接池、查询路由、水平分片和在线 DDL 等能力。Vitess 在 YouTube 每天处理数十亿条 SQL 查询，支撑了全球最大的 MySQL 部署之一。它通过 VTGate 代理层对应用透明地分发查询，通过 VTTablet 管理每个 MySQL 实例，并通过 VReplication 实现跨分片的数据同步和迁移。

## 核心能力

- **水平分片**: 自动化数据分片和路由，支持 hash、range 等分片策略，无需修改应用代码
- **连接池**: VTGate 高效的连接复用，保护后端 MySQL 不被过多连接压垮
- **查询路由**: 智能查询分发，将查询定向到正确的分片，支持跨分片查询合并
- **在线 DDL**: 无停机表结构变更（Online DDL），支持 gh-ost 和 pt-osc 策略
- **VReplication**: 高性能数据迁移和同步引擎，支持分片拆分、合并和迁移
- **备份恢复**: 自动化备份和时间点恢复（PITR），支持本地和云存储后端

## 架构

Vitess 采用分层代理架构，核心组件包括：

- **VTGate**: 轻量级无状态代理，接收 SQL 查询并路由到正确的 VTTablet，支持多租户
- **VTTablet**: 部署在每个 MySQL 实例旁的 sidecar 进程，管理 MySQL 生命周期、执行查询、处理复制
- **Topology Service**: 基于 etcd 的元数据存储，保存分片拓扑、VSchema 和路由信息
- **VTOrc**: 自动故障检测和修复组件，负责 MySQL 复制拓扑的管理和高可用切换
- **vtctld**: 管理界面，提供 Web UI 和 CLI 用于集群运维操作

数据流：`Application → VTGate (SQL 解析/路由) → VTTablet (查询执行) → MySQL`

## K8s 集成

Vitess 通过 **Vitess Operator** 实现与 Kubernetes 的深度集成。Operator 管理 VTGate、VTTablet 和 MySQL 的生命周期，使用 CRD（`EtcdLockserver`、`VitessCluster`）声明式定义集群拓扑。VTTablet 作为 sidecar 与 MySQL 运行在同一 Pod 中，通过本地 Unix socket 通信。Vitess Operator 支持自动故障转移、滚动升级、备份调度和分片重平衡，全部以 Kubernetes 原生方式管理。生产环境推荐配合 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 PodDisruptionBudget 和 topologySpreadConstraints 使用。

## 生产场景

1. **大规模 MySQL 分片**: 单表数据量超过单机容量时，通过 Vitess 水平分片将数据分散到多个 MySQL 实例
2. **多租户 SaaS 平台**: 利用 Vitess 的多租户能力为不同租户提供隔离的数据库访问
3. **零停机数据库迁移**: 使用 VReplication 在不同分片策略之间迁移数据，无需停服
4. **云原生 MySQL 高可用**: 在 Kubernetes 上运行 MySQL，借助 Operator 实现自动化运维

## 安装与配置

```bash
# 安装 Vitess Operator
kubectl apply -f https://raw.githubusercontent.com/planetscale/vitess-operator/v2.10.0/deploy/operator.yaml
# 部署示例集群
kubectl apply -f https://raw.githubusercontent.com/planetscale/vitess-operator/v2.10.0/examples/local/example.yaml
# 验证部署
kubectl get pods -n vitess
kubectl get vitessclusters

# 安装 vtctlclient CLI
brew install vitess
# 连接集群
vtctlclient --server localhost:15999 ListAllTablets
```

```yaml
# VitessCluster CRD 示例
apiVersion: planetscale.com/v2
kind: VitessCluster
metadata:
  name: prod-cluster
spec:
  cells:
    - name: zone1
      gateway:
        replicas: 2
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
  keyspaces:
    - name: commerce
      turndownPolicy: RequireIdle
      partitionings:
        - equal:
            parts: 2
            shardTemplate:
              databaseInitScriptSecret:
                name: commerce-schema
                key: init_db.sql
              tabletPools:
                - cell: zone1
                  type: replica
                  replicas: 2
                  vttablet:
                    resources:
                      requests:
                        cpu: "1"
                        memory: 1Gi
                  mysqld:
                    resources:
                      requests:
                        cpu: "1"
                        memory: 2Gi
```

```bash
# 连接 VTGate 执行 SQL
mysql -h <vtgate-svc> -P 15306 -u app_user
> SELECT * FROM users WHERE id = 1;
> SHOW VITESS_SHARDS;
```

## 运维操作

```bash
# 🟢 查看集群状态
kubectl get vitessclusters
kubectl get vitesskeyspaces
kubectl get vitessshards
vtctlclient --server localhost:15999 GetSchema commerce

# 🟢 查看分片和 Tablet 状态
vtctlclient --server localhost:15999 ListAllTablets
vtctlclient --server localhost:15999 GetShard commerce/-80

# 🟢 检查复制状态
vtctlclient --server localhost:15999 ShardReplicationPositions commerce/-80

# 🟡 在线 DDL
vtctlclient --server localhost:15999 ApplySchema -sql="ALTER TABLE users ADD COLUMN email VARCHAR(255)" -ddl_strategy=online commerce

# 🟡 分片重平衡 (Reshard)
vtctlclient --server localhost:15999 Reshard commerce/-80,80- commerce/-40,40-80,80-c0,c0-

# 🔴 故障转移 (PlannedReparentShard)
vtctlclient --server localhost:15999 PlannedReparentShard -keyspace_shard commerce/-80
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 查询路由错误 | VSchema 配置不正确 | `vtctlclient GetVSchema commerce` | 更新 VSchema 分片键 |
| Tablet 不可用 | MySQL 崩溃/资源不足 | `kubectl get pods -n vitess` | 检查 Pod 状态和资源限制 |
| 复制延迟高 | 大事务/网络延迟 | `vtctlclient ShardReplicationPositions` | 检查大事务和网络 |
| VTGate 连接拒绝 | VTGate Pod 未就绪 | `kubectl logs -n vitess -l app=vtgate` | 检查 VTGate 资源和配置 |
| 备份失败 | 存储后端不可达 | `kubectl logs -n vitess -l app=vtbackup` | 检查 S3/GCS 凭据和网络 |

```
排查流程：
├─ 查询问题
│  ├─ 检查 VSchema 分片键配置
│  ├─ vtctlclient Validate 验证拓扑
│  └─ 检查 VTGate 日志
├─ Tablet/MySQL 问题
│  ├─ kubectl get pods 检查 Pod 状态
│  ├─ 检查 MySQL 复制状态
│  └─ 检查磁盘空间和资源
└─ 高可用问题
   ├─ 检查 VTOrc 状态
   ├─ 检查 etcd Topology 一致性
   └─ 验证故障转移流程
```

## 生产案例

### 案例 1：YouTube 级别 MySQL 分片

- **场景**: 单表数据量达 10TB，单机 MySQL 无法承载
- **排查**: 评估分片方案，需要透明分片不修改应用代码
- **方案**: Vitess 水平分片为 16 个 shard，VTGate 透明路由查询
- **效果**: 查询延迟保持 <10ms，支撑数十亿日查询

### 案例 2：零停机分片拆分

- **场景**: 业务增长需要将 2 个 shard 拆分为 4 个，不能停服
- **排查**: 传统分片拆分需要停服迁移数据
- **方案**: Vitess VReplication + Reshard 在线拆分，双写期间自动同步
- **效果**: 零停机完成分片拆分，业务无感知

## 对比

| 维度 | Vitess | ProxySQL | MySQL Router | TiDB |
|------|--------|----------|--------------|------|
| 分片能力 | ✅ 原生水平分片 | ❌ 仅代理 | ❌ 仅代理 | ✅ 分布式 |
| 在线 DDL | ✅ 支持 | ❌ | ❌ | ✅ |
| MySQL 兼容 | ✅ 完全 | ✅ | ✅ | ⚠️ 部分 |
| CNCF 状态 | Graduated | 非 CNCF | 非 CNCF | 非 CNCF |
| 适用场景 | 大规模 MySQL | 读写分离 | 简单路由 | 分布式 HTAP |

## 架构定位

在 CNCF 生态中，Vitess 属于 **Database** 类别，为云原生应用提供关键的 MySQL 水平扩展和集群管理能力。

## 参考链接

- [[etcd]]
- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[tinkerbell]] — Tinkerbell
- [[sops]] — SOPS (Secrets OPerationS) OPerationS)
- [[23-实体/08-交付与制品/argocd.md|argocd]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- vitess
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
