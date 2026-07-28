---
title: OpenEBS [entities]
description: '## 概述'
summary: 'OpenEBS 是领先的容器原生存储解决方案，将存储控制器作为容器运行，实现了存储的容器化和微服务化。它提供多种存储引擎，支持本地存储 (Local PV) 和分布式复制存储 (Replicated PV)，适用于有状态应用的各种场景。'
category: entities
tags:
- k8s
- cncf
- storage
- openebs
- prometheus
- grafana
- rook
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenEBS 是什么
- 如何 OpenEBS
trigger_keywords:
- OpenEBS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenEBS

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

OpenEBS 是领先的容器原生存储（Container-Native Storage）解决方案，由 MayaData 开发，2019 年加入 CNCF Sandbox。它将存储控制器作为容器运行，实现了存储的容器化和微服务化。OpenEBS 提供多种存储引擎，支持本地存储（Local PV）和分布式复制存储（Replicated PV/Mayastor），适用于有状态应用的各种场景。它是 Kubernetes 生态中使用最广泛的开源 CSI 存储项目之一。

## 核心特性

- **容器原生架构**: 存储控制器和数据平面均以 Pod 形式运行
- **多存储引擎**: Local PV（Hostpath/Device/RAM）、Mayastor（SPDK 高性能）、cStor
- **CSI 原生**: 完全基于 Container Storage Interface 实现
- **快照与克隆**: 支持 VolumeSnapshot 和 PVC Clone
- **备份恢复**: 集成 Velero 实现灾难恢复
- **监控集成**: 内置 Prometheus 指标和 Grafana 仪表盘

## 架构

OpenEBS 采用微服务存储架构。核心组件包括：MayaStor（高性能存储引擎，基于 SPDK 用户态块设备）、Local PV（直接使用节点磁盘，零开销）、cStor（基于 ZFS 的复制存储引擎）。每个存储卷对应一个 Target Pod（iSCSI/NVMe-oF Target）和多个 Replica Pod。Target Pod 接收来自 CSI Plugin 的 I/O 请求，同步写入 Replica。Provisioner 监听 PVC 创建请求，自动分配存储和创建 Target/Replica。Mayactor Operator 管理存储池和卷的生命周期。

## Kubernetes 集成

OpenEBS 通过 CSI Driver 与 Kubernetes 集成。部署为 DaemonSet（mayastor、node operator）和 Deployment（provisioner、API server）。StorageClass 定义使用哪个 OpenEBS 引擎和参数。支持标准的 PVC → PV 映射、VolumeSnapshot 和 Clone。Local PV 模式直接使用节点磁盘，无网络开销，适合需要极低延迟的数据库。Mayastor 使用 NVMe-oF 协议提供跨节点复制能力。

## 生产使用场景

1. **数据库存储**: 为 PostgreSQL、MongoDB 等 StatefulSet 提供高性能持久卷
2. **本地存储加速**: 使用 Local PV 直连 NVMe/SSD，实现最高 IOPS
3. **Dev/Test 环境**: 在共享集群上为每个团队提供隔离的存储空间
4. **Kafka/Elasticsearch**: 为分布式消息队列和搜索引擎提供复制存储

## 安装与配置

```bash
# Helm 安装
helm repo add openebs https://openebs.github.io/openebs
helm install openebs openebs/openebs -n openebs --create-namespace
# 使用 Local PV
kubectl apply -f - <<EOF
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-local
provisioner: openebs.io/local
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
EOF
# 或使用 Mayastor
kubectl apply -f https://raw.githubusercontent.com/openebs/mayastor/master/deploy/mayastor.yaml
```

### Mayastor 高性能存储配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: mayastor-replicated
provisioner: io.openebs.csi-mayastor
parameters:
  protocol: nvmf
  repl_count: "3"  # 3 副本
  ioTimeout: "30"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
---
# PVC 示例
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
spec:
  storageClassName: mayastor-replicated
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

### 存储池配置

```yaml
apiVersion: openebs.io/v1beta2
kind: DiskPool
metadata:
  name: pool-1
  namespace: openebs
spec:
  node: worker-1
  disks:
    - /dev/sdb
    - /dev/sdc
```

## 运维操作

```bash
# 🟢 查看存储池状态
kubectl get diskpools -n openebs

# 🟢 查看 PV 状态
kubectl get pvc -A
kubectl describe pvc postgres-data

# 🟢 查看 Mayastor 卷
kubectl get msv -n openebs

# 🟢 检查 CSI 插件状态
kubectl get pods -n openebs -l app=openebs

# 🟡 扩容 PVC
kubectl patch pvc postgres-data -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 🟡 创建快照
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: postgres-snap
spec:
  volumeSnapshotClassName: mayastor-snapshot
  source:
    persistentVolumeClaimName: postgres-data
EOF

# 🟢 查看存储指标
kubectl exec -n openebs deploy/mayastor-api-rest -- curl -s localhost:8080/v0/nexus
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| PVC Pending | 无可用存储池 | `kubectl describe pvc` | 创建 DiskPool 或检查节点 |
| Pod 挂载失败 | CSI 插件异常 | `kubectl logs -n openebs ds/mayastor` | 重启 CSI DaemonSet |
| I/O 延迟高 | 网络拥塞 | `kubectl exec deploy/mayastor-api-rest -- ...` | 检查 NVMe-oF 网络 |
| 副本不同步 | 节点故障 | `kubectl get msv -o yaml` | 检查副本状态和节点健康 |
| 扩容失败 | 存储池空间不足 | `kubectl get diskpools -o yaml` | 添加磁盘或扩展存储池 |

### 排查流程

```
OpenEBS 存储异常
├─ PVC 无法绑定？
│  ├─ StorageClass 不存在 → 检查 SC 名称
│  ├─ 无可用存储池 → 创建 DiskPool
│  └─ 容量不足 → 扩展存储池
├─ Pod 挂载失败？
│  ├─ CSI 插件异常 → 检查 DaemonSet 状态
│  ├─ 节点无磁盘 → 检查 DiskPool 节点分配
│  └─ 权限问题 → 检查 SecurityContext
└─ 性能问题？
   ├─ IOPS 低 → 检查 NVMe/SSD 和 SPDK 配置
   ├─ 延迟高 → 检查网络（NVMe-oF）
   └─ 副本同步慢 → 检查节点间带宽
```

## 生产案例

### 案例 1: PostgreSQL 高可用存储

**场景**: 生产 PostgreSQL 需要高性能、高可用的持久存储。

**方案**:
1. 使用 Mayastor 3 副本存储
2. NVMe-oF 协议提供低延迟
3. VolumeSnapshot 实现定期备份
4. PVC Clone 快速创建测试环境

**效果**: IOPS 达 100K+，故障切换 < 30s，备份恢复 < 5min。

### 案例 2: Kafka 本地存储加速

**场景**: Kafka 需要极高吐吐量的本地存储。

**方案**:
1. 使用 Local PV (Hostpath) 直连 NVMe
2. 零网络开销，最大化 I/O 性能
3. 依赖 Kafka 自身副本机制保证可用性

**效果**: 吐吐量提升 3x，延迟降低 50%。

## 对比与替代方案

| 维度 | OpenEBS | Longhorn | Rook/Ceph | TopoLVM |
|------|---------|----------|-----------|----------|
| 存储引擎 | 多引擎 | 块存储 | Ceph | LVM |
| 性能 | 高 (Mayastor) | 中 | 中 | 高 |
| 部署复杂度 | 中 | 低 | 高 | 低 |
| 快照/克隆 | ✅ | ✅ | ✅ | ✅ |
| 多租户 | ✅ | ❌ | ✅ | ❌ |
| UI | ❌ | ✅ | ✅ | ❌ |

## 检查清单

- [ ] 存储引擎选择已评估（Local PV vs Mayastor）
- [ ] DiskPool 已创建并有足够容量
- [ ] StorageClass 已配置并测试
- [ ] 副本数已配置（生产建议 3）
- [ ] VolumeSnapshot 已配置定期备份
- [ ] 监控告警：存储池使用率 > 80%
- [ ] 扩容流程已测试
- [ ] 故障恢复演练已完成

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **OpenEBS** | 多引擎、CSI 原生 | 引擎选择复杂、cStor 性能一般 |
| Longhorn | 部署简单、UI 友好 | 仅块存储、性能不如 Mayastor |
| Rook/Ceph | 功能最全面 | 资源开销大、运维复杂 |
| TopoLVM | 高性能 LVM | 功能较少 |

## 架构定位

在 CNCF 生态中，OpenEBS 属于 **Storage** 类别，是容器原生存储的代表性项目。它的多引擎架构使其能适配从本地高性能到分布式复制等多种场景。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[karmada]] — Karmada
- [[rook]] — Rook
- [[microcks]] — Microcks
- [[keylime]] — Keylime
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openebs
- [[23-实体/cncf-storage.md|[[23-实体/15-参考与索引/cncf-storage|CNCF 存储与数据库项目全景]]]] — Cross-reference
- [[21-生态参考/03-领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[21-生态参考/03-领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
