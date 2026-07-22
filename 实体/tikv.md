---
title: TiKV (entities)
description: '## 概述'
summary: 'TiKV 是一个分布式事务 Key-Value 数据库，由 PingCAP 开发，CNCF 毕业项目。'
category: entities
tags:
- k8s
- cncf
- storage
- tikv
- scheduler
- prometheus
- grafana
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- TiKV 是什么
- 如何 TiKV
trigger_keywords:
- TiKV
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# TiKV

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Rust

## 概述

TiKV 是一个分布式事务 Key-Value 数据库，由 PingCAP 开发，2018 年加入 CNCF 孵化，2020 年正式毕业（Graduated）。它提供完整的 ACID 事务支持和水平扩展能力，最初作为 TiDB（分布式 SQL 数据库）的底层存储引擎设计，后来发展为独立的分布式 KV 数据库。TiKV 基于 Google Percolator 事务模型和 Multi-Raft 共识协议，支持强一致性分布式事务、自动数据分片（Region）和负载均衡。它提供 RawKV（无事务 KV API）和 TxnKV（事务 KV API）两种接口模式，适合需要高吞吐、低延迟和强一致性的数据存储场景。TiKV 在金融、电商、游戏等领域有广泛的生产应用。

## 核心能力

- **分布式事务**: 完整的 ACID 事务支持，基于 Google Percolator 模型，支持跨分片事务
- **水平扩展**: 自动数据分片（Region，默认 96MB），自动分裂和迁移
- **高可用**: Multi-Raft 共识协议，自动故障检测和主副本切换
- **强一致性**: 线性一致读写，支持快照隔离（Snapshot Isolation）
- **协处理器**: 下推计算能力，在存储节点执行过滤/聚合，减少数据传输
- **RawKV/TxnKV**: 支持原始 KV 和事务 KV 两种 API 模式

## 架构

TiKV 采用分层分布式存储架构：

- **TiKV Node**: 存储节点，管理数据 Region 的 Raft 组
- **Region**: 数据分片单元（默认 96MB），每个 Region 有 3 副本（Raft）
- **Raft Group**: 每个 Region 一个 Raft 组，保证副本一致性
- **RocksDB**: 单机存储引擎（LSM-Tree），每个 TiKV 节点有两个 RocksDB 实例（write CF + default CF）
- **PD (Placement Driver)**: 全局调度器，管理 Region 分布、负载均衡和事务时间戳分配
- **Titan (可选)**: 大 Value 优化引擎，分离存储大 Value 减少 LSM 放大

数据流：`客户端 → PD (Region 路由) → TiKV Leader (Raft 写入) → 副本同步 → 提交`

## K8s 集成

TiKV 通过 TiDB Operator（或独立的 TiKV Operator）与 Kubernetes 集成。TiKV 集群通过 CRD 声明式定义（TidbCluster CRD），Operator 管理 TiKV、PD 和 TiDB 组件的生命周期。TiKV Pod 以 StatefulSet 运行，使用 Local PV 或网络存储提供高性能持久化。PD 负责 Region 调度——在 Pod 扩缩容时自动迁移 Region。Operator 支持滚动升级、自动故障恢复和备份恢复。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 StatefulSet、PV/PVC 和 PodDisruptionBudget 集成。

## 生产场景

1. **分布式 SQL 数据库**: 作为 TiDB 的底层存储引擎，支撑大规模 OLTP 业务
2. **高吞吐 KV 存储**: 作为独立的分布式 KV 数据库，支撑缓存、计数器等场景
3. **实时分析**: 配合 TiFlash 列式存储，实现 HTAP（事务+分析）混合负载
4. **消息队列存储**: 作为 Kafka/Pulsar 的底层持久化存储

## 安装与配置

```bash
# 安装 TiDB Operator
helm repo add pingcap https://charts.pingcap.org/
helm install tidb-operator pingcap/tidb-operator -n tidb-admin --create-namespace
kubectl get pods -n tidb-admin
```

### TiDB 集群配置（包含 TiKV）

```yaml
apiVersion: pingcap.com/v1alpha1
kind: TidbCluster
metadata:
  name: basic
  namespace: tidb-cluster
spec:
  version: v7.5.0
  timezone: Asia/Shanghai
  pd:
    replicas: 3
    requests:
      storage: "10Gi"
  tikv:
    replicas: 3
    requests:
      storage: "100Gi"
    config:
      storage:
        engine: raftkv
      raftstore:
        sync-log: true
  tidb:
    replicas: 2
```

### 独立 TiKV 集群 (RawKV)

```yaml
apiVersion: tikv.org/v1alpha1
kind: TikvCluster
metadata:
  name: rawkv-cluster
spec:
  version: v7.5.0
  pd:
    replicas: 3
  tikv:
    replicas: 3
    requests:
      storage: "100Gi"
```

## 运维操作

```bash
# 🟢 查看集群状态
kubectl get tc -n tidb-cluster
kubectl exec -n tidb-cluster basic-tidb-0 -- tiup client

# 🟢 查看 TiKV 节点状态
kubectl get pods -n tidb-cluster -l app.kubernetes.io/component=tikv

# 🟡 扩容 TiKV
kubectl patch tc basic -n tidb-cluster --type=merge -p '{"spec":{"tikv":{"replicas":5}}}'

# 🟡 滚动升级
kubectl patch tc basic -n tidb-cluster --type=merge -p '{"spec":{"version":"v7.6.0"}}'

# 🔴 缩容 TiKV（需先驱逐 leader）
kubectl patch tc basic -n tidb-cluster --type=merge -p '{"spec":{"tikv":{"replicas":2}}}'
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| TiKV Pod CrashLoop | 磁盘空间不足 | `kubectl describe pod tikv-pod` | 扩容 PVC |
| Region 不可用 | 多数副本丢失 | `pd-ctl region check` | 恢复节点/重建副本 |
| 写入延迟高 | Raft 日志同步慢 | `tikv-ctl --host tikv:20160 metrics` | 检查磁盘 IO |
| PD 调度异常 | Leader 不均衡 | `pd-ctl store` | 手动调度/检查网络 |
| 连接超时 | TiKV 过载 | `kubectl top pod` | 扩容/限流 |

```
排查流程:
├── 集群异常
│   ├── kubectl get tc -o yaml → 查看状态
│   ├── kubectl logs tikv-pod → TiKV 日志
│   └── pd-ctl cluster → PD 集群状态
├── 性能问题
│   ├── Grafana TiKV 面板 → 延迟/吞吐
│   ├── tikv-ctl metrics → 内部指标
│   └── 检查磁盘 IO 和网络延迟
└── 数据异常
    ├── pd-ctl region check → Region 状态
    └── 确认副本数和分布
```

## 生产案例

### 案例 1: 在线扩容零停机

- **场景**: 业务增长需要扩容 TiKV 从 3 节点到 6 节点
- **方案**: 通过 TiDB Operator patch replicas；PD 自动调度 Region 到新节点；监控 rebalance 进度
- **效果**: 扩容全程业务无感知，Region 均衡时间 2h

### 案例 2: 磁盘故障数据恢复

- **场景**: 单节点磁盘故障，该节点上 200+ Region 副本丢失
- **方案**: Raft 多副本自动恢复；PD 调度补充副本；替换故障磁盘后重新加入
- **效果**: 数据零丢失，完全恢复时间 4h

## 对比

| 特性 | TiKV | etcd | Redis Cluster | Cassandra | 适用场景 |
|------|------|------|---------------|-----------|----------|
| 事务 | ✅ ACID | ⚠️ 简单 | ❌ | ⚠️ 轻量 | 强一致 |
| 强一致性 | ✅ | ✅ | ❌ 最终 | ⚠️ | 金融 |
| 水平分片 | ✅ 自动 | ❌ | ✡ 手动 | ✅ | 大规模 |
| 容量 | PB 级 | GB 级 | TB 级 | PB 级 | 数据量 |
| CNCF 状态 | Graduated | Graduated | 非 CNCF | 非 CNCF | 生态 |

## 架构定位

在 CNCF 生态中，TiKV 属于 **Storage** 类别，为云原生应用提供分布式事务 Key-Value 存储能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/storage-model.md|storage-model]]
- [[实体/kube-scheduler.md|kube-scheduler]]

## Related

- [[生态参考/98-merged-indexes/index.md|release-notes-observability]] — 发布说明索引 — 可观测性
- [[实体/cncf-observability.md|cncf-observability]] — CNCF 可观测性项目全景
- [[chaos-mesh]] — Chaos Mesh
- [[kubean]] — Kubean
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tikv
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
