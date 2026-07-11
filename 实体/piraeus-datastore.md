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

Piraeus 通过 Piraeus Operator 部署在 Kubernetes 集群中。Operator 管理 LINSTOR Controller 和 Satellite（DaemonSet）的生命周期，自动在存储节点上加载 DRBD 内核模块。CSI Driver（piraeus-csi）通过标准 CSI 接口为 Kubernetes 提供存储供应——StorageClass 定义存储池和副本配置，PVC 创建时 LINSTOR 自动创建 DRBD 卷。Pod 调度到节点后，CSI Node 将 DRBD 设备挂载到 Pod 容器中。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 PV/PVC/StorageClass 标准机制完全兼容。

## 生产场景

1. **数据库高可用**: MySQL/PostgreSQL 等数据库使用 DRBD 同步复制，RPO=0
2. **多可用区存储**: 在多个 AZ 部署 DRBD 副本，实现跨 AZ 高可用
3. **灾难恢复**: 通过异步复制将数据同步到远端集群
4. **合规加密**: 使用 LUKS 加密满足数据加密合规要求

## 安装

```bash
# Helm 安装 Piraeus Operator
helm repo add piraeus https://piraeus.io/piraeus/
helm install piraeus piraeus/piraeus -n piraeus-datastore --create-namespace

# 确保节点已加载 DRBD 内核模块
# modprobe drbd

# 创建 LINSTOR StorageClass
kubectl apply -f - <<EOF
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
EOF

# 创建 PVC
kubectl apply -f - <<EOF
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
EOF
```

## 对比

| 特性 | Piraeus | Longhorn | OpenEBS | Ceph RBD |
|------|---------|----------|---------|----------|
| 同步复制 | ✅ DRBD | ✅ | ⚠️ | ✅ |
| RPO | 0 | 0 | >0 | 0 |
| 性能 | 高（内核态） | 中 | 中 | 中 |
| CNCF 状态 | Sandbox | CNCF | Sandbox | Graduated |

## 架构定位

在 CNCF 生态中，Piraeus 属于 **Storage** 类别，为云原生应用提供基于 DRBD 的高可用块存储能力。

## 参考链接

- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[实体/csi-drivers.md|csi-drivers]]

## Related

- [[spin]] — Spin
- [[backstage]] — Backstage
- [[实体/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- piraeus-datastore
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
