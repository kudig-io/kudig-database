---
title: Piraeus Datastore (entities)
description: '## 概述'
summary: 'Piraeus Datastore 是基于 LINSTOR 和 DRBD 技术的 Kubernetes 高可用存储解决方案。'
category: entities
tags:
- k8s
- cncf
- storage
- piraeus-datastore
- ingress
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
- Piraeus Datastore 是什么
- 如何 Piraeus Datastore
trigger_keywords:
- Piraeus
- Datastore
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Piraeus Datastore

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go, Java

## 概述

Piraeus Datastore 是基于 LINSTOR 和 DRBD 技术的 Kubernetes 高可用存储解决方案，由 LINBIT 开发，2021 年加入 CNCF 沙箱。它提供高性能的块存储，支持同步复制、快照、加密和灾难恢复。Piraeus 将成熟的 Linux 存储技术（DRBD 同步复制已有 20+ 年历史）与 Kubernetes 原生体验结合，为有状态应用提供企业级存储。DRBD（Distributed Replicated Block Device）在内核态实现块设备级别的同步复制，数据同时写入主节点和一个或多个副本节点，提供 RPO=0 的数据保护能力。Piraeus 通过 LINSTOR 管理存储池、卷和复制拓扑，通过 CSI 接口为 Kubernetes 提供动态存储供应。

## 核心能力

- **DRBD 同步复制**: 内核态块设备级同步复制，RPO=0，主节点故障时副本立即可用
- **LINSTOR 管理**: 集中管理存储节点、存储池、卷和复制拓扑
- **高可用**: 主副本故障时自动提升副本为主，实现 RTO≈0
- **快照**: 基于快照的备份和时间点恢复
- **透明加密**: 支持 LUKS 透明数据加密
- **多存储后端**: 支持 LVM、ZFS、OpenEBS 等多种后端

## 架构

Piraeus 基于 LINSTOR + DRBD 双层架构：

- **Piraeus Operator**: 管理 LINSTOR 和 CSI 组件的 Kubernetes Operator
- **LINSTOR Controller**: 集群级控制器，管理存储资源（节点、池、卷定义）
- **LINSTOR Satellite**: 部署在每个存储节点上的 Agent，执行本地存储操作
- **DRBD 内核模块**: 实现块设备同步复制的 Linux 内核模块
- **CSI Controller (piraeus-csi)**: CSI Controller 服务，处理 PV 创建/删除/扩容
- **CSI Node (piraeus-csi-node)**: 每个节点上的 DaemonSet，处理 PV 挂载/卸载
- **Storage Pool**: 节点上的物理存储池（LVM VolumeGroup 或 ZFS zpool）

数据流：`Pod → PV → DRBD 设备 → 同步复制 → 副本节点 DRBD 设备`

## K8s 集成

Piraeus 通过 Piraeus Operator 部署在 Kubernetes 集群中。Operator 管理 LINSTOR Controller 和 Satellite（DaemonSet）的生命周期，自动在存储节点上加载 DRBD 内核模块。CSI Driver（piraeus-csi）通过标准 CSI 接口为 Kubernetes 提供存储供应——StorageClass 定义存储池和副本配置，PVC 创建时 LINSTOR 自动创建 DRBD 卷。Pod 调度到节点后，CSI Node 将 DRBD 设备挂载到 Pod 容器中。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 PV/PVC/StorageClass 标准机制完全兼容。

## 生产场景

1. **数据库高可用**: MySQL/PostgreSQL 等数据库使用 DRBD 同步复制，RPO=0
2. **多可用区存储**: 在多个 AZ 部署 DRBD 副本，实现跨 AZ 高可用
3. **灾难恢复**: 通过异步复制将数据同步到远端集群
4. **合规加密**: 使用 LUKS 加密满足数据加密合规要求

## 安装与配置

```bash
# Helm 安装 Piraeus Operator
helm repo add piraeus https://piraeus.io/piraeus/
helm install piraeus piraeus/piraeus -n piraeus-datastore --create-namespace

# 确保节点已加载 DRBD 内核模块
# modprobe drbd
kubectl get pods -n piraeus-datastore
```

### StorageClass 配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: piraeus-sc
provisioner: piraeus.csidriver.io
parameters:
  csi.storage.k8s.io/fstype: ext4
  autoPlace: "2"
  replication: "sync"
  encryption: "false"
allowVolumeExpansion: true
reclaimPolicy: Delete
---
# 加密版本
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: piraeus-encrypted
provisioner: piraeus.csidriver.io
parameters:
  csi.storage.k8s.io/fstype: ext4
  autoPlace: "3"
  replication: "sync"
  encryption: "true"
```

### PVC 创建

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: piraeus-pvc
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: piraeus-sc
  resources:
    requests:
      storage: 50Gi
```

## 运维操作

```bash
# 🟢 查看 LINSTOR 资源状态
kubectl exec -n piraeus-datastore deploy/piraeus-controller -- linstor resource list

# 🟢 查看存储池
kubectl exec -n piraeus-datastore deploy/piraeus-controller -- linstor storage-pool list

# 🟡 扩容 PVC
kubectl patch pvc piraeus-pvc -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 🟡 添加存储节点
kubectl apply -f new-node.yaml

# 🔴 删除资源（数据不可恢复）
kubectl exec -n piraeus-datastore deploy/piraeus-controller -- linstor resource delete <node> <resource>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| PVC Pending | DRBD 模块未加载 | `lsmod | grep drbd` | `modprobe drbd` |
| 复制失败 | 节点间网络不通 | `linstor resource list` | 检查节点间网络 |
| IO 延迟高 | 磁盘过载 | `iostat -x 1` | 检查磁盘健康/扩容 |
| CSI Pod CrashLoop | 内核版本不兼容 | `kubectl logs csi-pod` | 升级内核/DRBD |
| 卷挂载失败 | 资源未就绪 | `kubectl describe pvc` | 检查 LINSTOR 状态 |

```
排查流程:
├── PVC 无法绑定
│   ├── kubectl describe pvc → Events
│   ├── lsmod | grep drbd → 确认内核模块
│   └── kubectl logs -n piraeus-datastore → Operator 日志
├── 复制异常
│   ├── linstor resource list → 查看同步状态
│   ├── 检查节点间网络连通性
│   └── dmesg | grep drbd → 内核日志
└── 性能问题
    ├── iostat -x 1 → 磁盘 IO
    ├── linstor resource list → 查看资源分布
    └── 确认副本数满足要求
```

## 生产案例

### 案例 1: 数据库高可用存储

- **场景**: PostgreSQL 需要 RPO=0 的同步复制存储
- **方案**: 使用 Piraeus 同步复制(autoPlace=2)；PostgreSQL 主备分别调度到不同节点；DRBD 保证数据实时同步
- **效果**: 节点故障时数据零丢失，故障转移时间 <30s

### 案例 2: 加密合规存储

- **场景**: 金融业务要求数据静态加密(LUKS)
- **方案**: 创建 encryption=true 的 StorageClass；所有敏感数据 PVC 使用该 SC
- **效果**: 通过合规审计，数据加密透明无感知

## 对比

| 特性 | Piraeus | Longhorn | OpenEBS | Ceph RBD | 适用场景 |
|------|---------|----------|---------|----------|----------|
| 同步复制 | ✅ DRBD | ✅ | ⚠️ | ✅ | RPO=0 |
| 性能 | 高（内核态） | 中 | 中 | 中 | 高性能 |
| 加密 | ✅ LUKS | ⚠️ | ❌ | ✅ | 合规 |
| 复杂度 | 中 | 低 | 中 | 高 | 运维成本 |
| CNCF 状态 | Sandbox | CNCF | Sandbox | Graduated | 生态 |

## 架构定位

在 CNCF 生态中，Piraeus 属于 **Storage** 类别，为云原生应用提供基于 DRBD 的高可用块存储能力。

## 参考链接

- [[operator-pattern]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[23-实体/02-K8s核心组件/csi-drivers.md|csi-drivers]]

## Related

- [[spin]] — Spin
- [[backstage]] — Backstage
- [[23-实体/04-网络/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- piraeus-datastore
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
