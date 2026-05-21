---
title: HwameiStor
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- mysql
- vpa
- crd
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HwameiStor 是什么
- 如何 HwameiStor
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- HwameiStor
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- mysql-basics
---

title: HwameiStor
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- mysql
- vpa
- crd
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- HwameiStor 是什么
- 如何 HwameiStor
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- HwameiStor
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# HwameiStor

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://hwameistor.io/ |
| **GitHub** | https://github.com/hwameistor/hwameistor |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

HwameiStor 是一个 Kubernetes 原生的高可用本地存储系统，能够将节点上的本地磁盘（HDD、SSD、NVMe）统一管理并提供分布式的本地存储服务。它通过 CSI 接口为有状态应用提供高性能的本地持久卷，并支持卷的高可用副本、数据迁移和自动化运维。HwameiStor 特别适合对 IOPS 和延迟敏感的工作负载，如数据库和 AI/ML 训练任务。

### 核心特性

- **高性能本地存储**: 直接使用节点本地磁盘，提供接近裸盘的 IOPS 和延迟
- **高可用卷 (HA)**: 支持卷数据的跨节点副本，节点故障时自动切换
- **磁盘池管理**: 自动发现和纳管节点上的磁盘，支持 HDD、SSD、NVMe 分类
- **卷扩容**: 在线扩展卷容量，无需停机
- **数据迁移**: 支持卷数据在节点间迁移，便于节点维护和负载均衡
- **Kubernetes 原生**: 完全通过 CRD 和 CSI 接口集成，声明式管理

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  HwameiStor Controller                │
│                                                       │
│  ┌──────────────┐ ┌───────────────┐ ┌─────────────┐  │
│  │ LocalDisk    │ │ LocalVolume   │ │ LocalVolumeReplica │
│  │ Manager      │ │ Controller    │ │ Controller   │  │
│  │ (磁盘发现)   │ │ (卷管理)      │ │ (副本管理)   │  │
│  └──────────────┘ └───────────────┘ └─────────────┘  │
└─────────────────────────┬────────────────────────────┘
                          │ CRD
┌─────────────────────────▼────────────────────────────┐
│           LocalDisk / LocalVolume CRDs                │
└────────────┬─────────────────────────────┬───────────┘
             │                             │
   ┌─────────▼──────────┐        ┌────────▼─────────┐
   │    Node 1           │        │    Node 2         │
   │  ┌───────────────┐  │        │  ┌───────────────┐│
   │  │ HwameiStor    │  │        │  │ HwameiStor    ││
   │  │ Agent         │  │        │  │ Agent         ││
   │  │ (CSI Node)    │  │        │  │ (CSI Node)    ││
   │  └───────┬───────┘  │        │  └───────┬───────┘│
   │  ┌───────▼───────┐  │        │  ┌───────▼───────┐│
   │  │ LVM / RAW     │  │        │  │ LVM / RAW     ││
   │  │ ┌───┐ ┌───┐   │  │◄──────►│  │ ┌───┐ ┌───┐  ││
   │  │ │SSD│ │HDD│   │  │ 副本   │  │ │SSD│ │HDD│  ││
   │  │ └───┘ └───┘   │  │        │  │ └───┘ └───┘  ││
   │  └───────────────┘  │        │  └───────────────┘│
   └─────────────────────┘        └──────────────────┘
```

---

## 快速开始

### 安装 HwameiStor

```bash
# 使用 Helm 安装
helm repo add hwameistor https://hwameistor.io/hwameistor
helm install hwameistor hwameistor/hwameistor \
  --namespace hwameistor-system \
  --create-namespace
```

### 查看发现的磁盘

```bash
# 查看节点上的磁盘
kubectl get localdisks
# NAME                NODE      DEVPATH        STATE      TYPE
# sda-node1           node1     /dev/sda       Available  SSD
# sdb-node1           node1     /dev/sdb       Available  HDD
# sda-node2           node2     /dev/sda       Available  SSD

# 查看磁盘详情
kubectl get localdisk sda-node1 -o yaml
```

### 创建存储池

```yaml
# storagepool.yaml
apiVersion: hwameistor.io/v1alpha1
kind: LocalStoragePool
metadata:
  name: ssd-pool
spec:
  poolClass: SSD
  disks:
    - localDisk: sda-node1
    - localDisk: sda-node2
```

### 创建 StorageClass

```yaml
# storageclass.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: hwameistor-lvm-ssd
provisioner: lvm.hwameistor.io
parameters:
  poolClass: SSD
  replicaNumber: "2"  # HA 副本数
  volumeKind: LVM
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 创建 PVC

```yaml
# pvc.yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: hwameistor-lvm-ssd
  resources:
    requests:
      storage: 100Gi
```

---

## 高级功能

### 高可用卷 (HA)

```yaml
# 创建 HA 卷的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: hwameistor-ha
provisioner: lvm.hwameistor.io
parameters:
  replicaNumber: "2"  # 2 副本
  poolClass: SSD
  # 副本拓扑约束
  topologyKey: "kubernetes.io/hostname"
```

```bash
# 查看卷副本状态
kubectl get localvolumereplicas
# NAME                       VOLUME       NODE    STATE
# mysql-data-replica-1       mysql-data   node1   Ready
# mysql-data-replica-2       mysql-data   node2   Ready
```

### 卷扩容

```bash
# 在线扩容 PVC
kubectl patch pvc mysql-data -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 查看扩容状态
kubectl get localvolume mysql-data -o yaml
```

### 数据迁移

```yaml
# 将卷从 node1 迁移到 node3
apiVersion: hwameistor.io/v1alpha1
kind: LocalVolumeMigrate
metadata:
  name: mysql-data-migrate
spec:
  volumeName: mysql-data
  sourceNodes:
    - node1
  targetNodes:
    - node3
```

### 磁盘管理

```bash
# 声明磁盘可用
kubectl patch localdisk sdc-node1 --type=merge \
  -p '{"spec":{"owner":"hwameistor"}}'

# 从存储池移除磁盘（需先迁移数据）
kubectl patch localdisk sda-node1 --type=merge \
  -p '{"spec":{"state":"Offline"}}'
```

---

## 与其他方案对比

| 特性 | HwameiStor | OpenEBS | TopoLVM | Longhorn |
|:---|:---|:---|:---|:---|
| 存储类型 | 本地 LVM | 多种 | 本地 LVM | 分布式 |
| HA 副本 | 支持 | cStor 支持 | 不支持 | 支持 |
| 性能 | 原生本地盘 | 取决于引擎 | 原生本地盘 | 网络开销 |
| 磁盘发现 | 自动 | 手动 | 手动 | N/A |
| 数据迁移 | 支持 | 支持 | 不支持 | 支持 |
| 扩容 | 在线 | 引擎相关 | 在线 | 在线 |

---

## 最佳实践

1. **磁盘分类**: 将 SSD 和 HDD 分配到不同存储池，按性能需求选择 StorageClass
2. **副本拓扑**: 配置副本拓扑约束确保副本分布在不同故障域
3. **监控告警**: 监控 LocalDisk 和 LocalVolume 的 conditions，及时发现故障
4. **定期迁移**: 在节点维护前提前进行卷迁移，减少业务影响
5. **资源预留**: 为每个节点预留足够的磁盘空间用于数据重建

---

## 参考资源

- [HwameiStor 官方文档](https://hwameistor.io/docs/)
- [HwameiStor GitHub](https://github.com/hwameistor/hwameistor)
- [HwameiStor 架构设计](https://hwameistor.io/docs/architecture)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
