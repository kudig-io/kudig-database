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

Carina 通过 CSI（Container Storage Interface）标准与 Kubernetes 集成。carina-controller 以 Deployment 运行，实现 CSI Controller RPC；carina-node 以 DaemonSet 运行在所有存储节点上，实现 CSI Node RPC。StorageClass 配置指定磁盘组（如 `carina.storage.io/disk-group-name: ssd`），PVC 创建时自动从对应 VolumeGroup 分配 LogicalVolume。Carina 通过 Kubernetes Scheduler Extender 或 CSI topology 能力实现存储感知调度。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Volume Attachment 机制完全兼容。

## 生产场景

1. **高性能数据库存储**: MySQL、PostgreSQL、Redis 等对 IO 延迟敏感的数据库使用本地 LVM 存储
2. **消息队列持久化**: Kafka、RabbitMQ 等高吞吐消息队列的本地存储供应
3. **AI/ML 训练数据**: GPU 节点本地 NVMe SSD 存储，加速模型训练数据读取
4. **成本敏感的大规模存储**: 相比网络存储（Ceph/NFS），本地存储成本更低且性能更高

## 安装与配置

### Helm 部署

```bash
# 安装 Carina CSI Driver
helm repo add carina https://carina-io.github.io/carina/
helm install carina carina/carina-csi-driver -n carina-system --create-namespace

# 验证部署
kubectl get pods -n carina-system
kubectl get storageclass | grep carina
```

### StorageClass 配置

```yaml
# SSD 磁盘组 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: carina-sc-ssd
provisioner: carina.storage.io
parameters:
  carina.storage.io/disk-group-name: ssd
  csi.storage.k8s.io/fstype: ext4
allowVolumeExpansion: true
reclaimPolicy: Delete
---
# HDD 磁盘组 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: carina-sc-hdd
provisioner: carina.storage.io
parameters:
  carina.storage.io/disk-group-name: hdd
  csi.storage.k8s.io/fstype: xfs
allowVolumeExpansion: true
```

### PVC 创建

```yaml
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
```

## 运维操作

```bash
# 🟢 查看 StorageClass
kubectl get storageclass | grep carina

# 🟢 查看 PVC 状态
kubectl get pvc -A | grep carina

# 🟢 查看节点磁盘组
kubectl get nodes -o wide
kubectl exec -n carina-system -it <carina-pod> -- carina get disk-groups

# 🟡 在线扩容 PVC
kubectl patch pvc carina-pvc -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 🟡 创建快照
kubectl apply -f volume-snapshot.yaml

# 🔴 删除 PVC（数据丢失）
kubectl delete pvc carina-pvc
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| PVC Pending | 磁盘组空间不足 | `kubectl describe pvc <name>` | 扩容磁盘组或减少请求 |
| Pod 无法挂载 | CSI 驱动异常 | `kubectl get pods -n carina-system` | 检查 CSI 驱动状态 |
| 扩容失败 | 文件系统不支持 | `kubectl describe pvc <name>` | 检查 fstype 和文件系统 |
| IO 性能差 | 磁盘组配置错误 | `iostat -x 1` | 检查磁盘组配置 |

**排查流程：**
```
PVC 创建失败
├── 检查 StorageClass → kubectl get storageclass carina-sc-ssd
├── 检查 CSI 驱动 → kubectl get pods -n carina-system
├── 检查节点磁盘 → kubectl exec -n carina-system <pod> -- carina get disk-groups
├── 检查 PVC 事件 → kubectl describe pvc <name>
└── 检查节点标签 → kubectl get node <node> --show-labels
```

## 生产案例

### 案例一：数据库本地存储

- **场景**: MySQL/PostgreSQL 需要高性能本地 SSD 存储
- **排查**: 网络存储延迟高，影响数据库性能
- **方案**: Carina 提供本地 SSD LVM 卷，延迟 < 1ms
- **效果**: 数据库 IOPS 提升 5x，延迟降低 80%

### 案例二：在线扩容

- **场景**: 业务增长需要在线扩容存储，不能停机
- **排查**: Carina 支持 LVM 在线扩容
- **方案**: 直接 patch PVC 大小，LVM 自动扩展
- **效果**: 扩容无需重启 Pod，业务零中断

## 对比

| 特性 | Carina | OpenEBS LocalPV | TopoLVM | Local Path | 适用场景 |
|------|--------|-----------------|---------|------------|----------|
| 存储引擎 | LVM | Bind mount / ZFS | LVM | 目录 | - |
| 在线扩容 | ✅ | ⚠️ 有限 | ✅ | ❌ | Carina/TopoLVM |
| 快照 | ✅ | ⚠️ ZFS only | ✅ | ❌ | - |
| IO 限速 | ✅ | ❌ | ✅ | ❌ | 多租户 |
| 磁盘组管理 | ✅ | ❌ | ⚠️ | ❌ | Carina 最强 |

## 架构定位

在 CNCF 生态中，Carina 属于 **Storage** 类别，为云原生应用提供基于 LVM 的高性能本地存储能力。

## 参考链接

- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[23-实体/02-K8s核心组件/kube-scheduler.md|kube-scheduler]]
- [[23-实体/02-K8s核心组件/csi-drivers.md|csi-drivers]]

## Related

- [[hexa]] — Hexa
- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- carina
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
