---
title: Carina (entities)
description: '## 概述'
summary: 'Carina 是一个 Kubernetes 本地存储供应器，基于 LVM（Logical Volume Manager）管理节点上的本地磁盘，为有状态应用提供高性能的本地持久化存储。'
category: entities
tags:
- k8s
- cncf
- storage
- carina
- scheduler
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Carina 是什么
- 如何 Carina
trigger_keywords:
- Carina
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Carina

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

Carina 是一个 Kubernetes 本地存储供应器，由博云（BoCloud）开源，2022 年加入 CNCF 沙箱。它基于 LVM（Logical Volume Manager）管理节点上的本地磁盘，为有状态应用提供高性能的本地持久化存储。Carina 自动发现节点上的裸盘，组建 LVM VolumeGroup，并通过 CSI（Container Storage Interface）接口为 Pod 动态分配 LogicalVolume 作为 PersistentVolume。与 OpenEBS、Local Path Provisioner 等本地存储方案相比，Carina 利用了成熟的 Linux LVM 技术，原生支持存储卷在线扩容、快照、克隆和 IO 限速，同时支持存储卷拓扑感知调度，将 Pod 调度到有足够存储容量的节点。

## 核心能力

- **LVM 本地存储**: 基于 LVM 管理裸盘，支持动态创建和删除 LogicalVolume
- **自动磁盘发现**: 自动发现节点上的裸盘并组建 VolumeGroup
- **在线扩容**: 无需重建 Pod 即可在线扩展 PV 容量
- **快照与克隆**: 基于 LVM snapshot 实现卷快照和克隆
- **IO 限速**: 支持对单个 LogicalVolume 设置 IOPS/BPS 限制
- **拓扑感知调度**: 通过 CSI topology 将 Pod 调度到有存储容量的节点

## 架构

Carina 采用 CSI 标准 + LVM 存储引擎架构：

- **Carina Controller (carina-controller)**: CSI Controller 服务，处理 PV 创建、删除、扩容等操作
- **Carina Node (carina-node)**: 部署在每个节点上的 DaemonSet，实现 CSI Node 服务，管理本地 LVM
- **Disk Discovery**: carina-node 启动时自动扫描裸盘，按设备类型（SSD/HDD）组建不同 VolumeGroup
- **LogicVolume CRD**: 每个分配的逻辑卷以 CRD 表示，记录设备路径、大小、VG 等信息
- **Scheduler Extender**: 自定义调度器扩展，过滤掉存储容量不足的节点
- **LVM 引擎**: 底层使用 lvcreate/lvextend/lvremove 管理 LogicalVolume

数据流：`PVC → Carina Controller → LogicVolume CRD → carina-node → lvcreate → /dev/vg/lv → Pod mount`

## K8s 集成

Carina 通过 CSI（Container Storage Interface）标准与 Kubernetes 集成。carina-controller 以 Deployment 运行，实现 CSI Controller RPC；carina-node 以 DaemonSet 运行在所有存储节点上，实现 CSI Node RPC。StorageClass 配置指定磁盘组（如 `carina.storage.io/disk-group-name: ssd`），PVC 创建时自动从对应 VolumeGroup 分配 LogicalVolume。Carina 通过 Kubernetes Scheduler Extender 或 CSI topology 能力实现存储感知调度。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Volume Attachment 机制完全兼容。

## 生产场景

1. **高性能数据库存储**: MySQL、PostgreSQL、Redis 等对 IO 延迟敏感的数据库使用本地 LVM 存储
2. **消息队列持久化**: Kafka、RabbitMQ 等高吞吐消息队列的本地存储供应
3. **AI/ML 训练数据**: GPU 节点本地 NVMe SSD 存储，加速模型训练数据读取
4. **成本敏感的大规模存储**: 相比网络存储（Ceph/NFS），本地存储成本更低且性能更高

## 安装

```bash
# Helm 安装 Carina
helm repo add carina https://carina-io.github.io/carina/
helm install carina carina/carina-csi-driver -n carina-system --create-namespace

# 创建 StorageClass（SSD 磁盘组）
kubectl apply -f - <<EOF
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: carina-sc-ssd
provisioner: carina.storage.io
parameters:
  carina.storage.io/disk-group-name: ssd
  csi.storage.k8s.io/fstype: ext4
EOF

# 创建 PVC
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: carina-pvc
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: carina-sc-ssd
  resources:
    requests:
      storage: 100Gi
EOF
```

## 对比

| 特性 | Carina | OpenEBS LocalPV | TopoLVM | Local Path |
|------|--------|-----------------|---------|------------|
| 存储引擎 | LVM | Bind mount / ZFS | LVM | 目录 |
| 在线扩容 | ✅ | ⚠️ 有限 | ✅ | ❌ |
| 快照 | ✅ | ⚠️ ZFS only | ✅ | ❌ |
| IO 限速 | ✅ | ❌ | ✅ | ❌ |

## 架构定位

在 CNCF 生态中，Carina 属于 **Storage** 类别，为云原生应用提供基于 LVM 的高性能本地存储能力。

## 参考链接

- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[实体/kube-scheduler.md|kube-scheduler]]
- [[实体/csi-drivers.md|csi-drivers]]

## Related

- [[hexa]] — Hexa
- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- carina
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
