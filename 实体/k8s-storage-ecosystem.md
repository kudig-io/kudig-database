---
title: 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
description: '# 存储体系'
summary: '# 存储体系'
category: reference
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- csi
- backup
- etcd
- scheduler
- ceph
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复 是什么
- 如何 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
trigger_keywords:
- 存储体系：PV
- PVC
- StorageClass
- CSI
- 驱动与灾备恢复
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 存储体系

> **CNCF 状态**: 生态概览 | **类别**: Storage | **主要语言**: YAML

## 概述

Kubernetes 存储生态系统是一个涵盖 CSI 驱动、存储类、卷快照、数据保护等多层面的综合技术体系。它定义了容器存储接口（CSI）标准，让存储厂商可以通过统一的插件接口为 K8s 提供持久化存储能力。该生态系统包括 CSI 驱动（如 Ceph RBD、NFS、EBS、Azure Disk）、存储编排工具（如 Rook、Longhorn）、数据保护方案（如 Velero、Kasten）等多个组件。

## Key Features（核心能力）

- **CSI 标准**：Container Storage Interface 统一了存储提供商的接入方式
- **StorageClass**：动态 PV 供应，支持存储分层和 QoS
- **VolumeSnapshot**：卷快照和恢复机制
- **Volume Expansion**：在线卷扩容
- **Rook/Longhorn**：K8s 原生的分布式存储编排
- **Velero**：集群资源和 PV 数据的备份恢复

## 架构与工作原理

K8s 存储生态由多个层级构成：存储介质层（块存储、文件存储、对象存储）；CSI 驱动层（Provisioner、Attacher、Snapshotter 三组件）；K8s API 层（PV/PVC/StorageClass/VolumeSnapshot CRD）；编排管理层（Rook operator、Longhorn manager）。PVC 通过 StorageClass 动态创建 PV，CSI 驱动与底层存储系统交互完成实际卷操作。

## K8s 集成

K8s 存储核心概念包括 PersistentVolume（PV，集群级存储资源）、PersistentVolumeClaim（PVC，用户级存储请求）、StorageClass（动态供应策略）。CSI 驱动通过 Sidecar 组件（external-provisioner、external-attacher、external-snapshotter）与 K8s 控制平面交互。Pod 通过 volumeMounts 引用 PVC，kubelet 通过 CSI gRPC 接口挂载卷到 Pod。

## 生产用例

- **数据库持久化**：MySQL/PostgreSQL 的持久化存储
- **消息队列存储**：Kafka/RabbitMQ 的数据卷
- **数据备份恢复**：Velero 定期备份 PV 数据到 S3
- **多区域存储**：跨 AZ 的存储复制和高可用

## 安装与配置

### Rook-Ceph 分布式存储部署

```bash
# 🟢 安装 Rook Operator
helm repo add rook-release https://charts.rook.io/release
helm repo update
helm install rook-ceph rook-release/rook-ceph \
  -n rook-ceph --create-namespace \
  --set crds.enabled=true

# 🟢 创建 Ceph 集群
kubectl apply -f - <<EOF
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: quay.io/ceph/ceph:v18.2
  dataDirHostPath: /var/lib/rook
  mon:
    count: 3
    allowMultiplePerNode: false
  mgr:
    count: 2
  storage:
    useAllNodes: false
    nodes:
    - name: "storage-node-1"
      devices:
      - name: "sdb"
      - name: "sdc"
    - name: "storage-node-2"
      devices:
      - name: "sdb"
      - name: "sdc"
EOF

# 🟢 创建 StorageClass
kubectl apply -f - <<EOF
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: replicapool
  imageFormat: "2"
  imageFeatures: layering
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/provisioner-secret-namespace: rook-ceph
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
  csi.storage.k8s.io/node-stage-secret-namespace: rook-ceph
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
EOF

# 🟢 验证存储集群状态
kubectl get cephcluster -n rook-ceph
kubectl get storageclass
```

### PVC 使用示例

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: rook-ceph-block
  resources:
    requests:
      storage: 50Gi
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ReadWriteOnce]
      storageClassName: rook-ceph-block
      resources:
        requests:
          storage: 100Gi
```

### VolumeSnapshot 配置

```yaml
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-rbdplugin-snapclass
driver: rook-ceph.rbd.csi.ceph.com
deletionPolicy: Retain
parameters:
  clusterID: rook-ceph
  csi.storage.k8s.io/snapshotter-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/snapshotter-secret-namespace: rook-ceph
---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-snapshot
spec:
  volumeSnapshotClassName: csi-rbdplugin-snapclass
  source:
    persistentVolumeClaimName: mysql-data
---
# 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-restored
spec:
  storageClassName: rook-ceph-block
  dataSource:
    name: mysql-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes: [ReadWriteOnce]
  resources:
    requests:
      storage: 50Gi
```

### Velero 备份配置

```bash
# 🟢 安装 Velero
velero install \
  --provider aws \
  --bucket k8s-backup \
  --secret-file ./credentials-velero \
  --use-volume-snapshots \
  --snapshot-location-config region=us-east-1

# 🟢 创建定时备份
velero schedule create daily-backup \
  --schedule="0 2 * * *" \
  --include-namespaces="production,database" \
  --ttl 168h  # 7天保留

# 🟢 查看备份状态
velero backup get
velero backup describe daily-backup-20260701
```

## 运维操作

```bash
# 🟢 检查存储状态
kubectl get pv,pvc -A
kubectl get storageclass
kubectl get volumesnapshot -A

# 🟢 检查 Ceph 集群健康
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph status
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph df
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- rados df

# 🟢 检查 CSI 驱动状态
kubectl get csidrivers
kubectl get csinodes
kubectl get pods -n rook-ceph -l app=csi-rbdplugin

# 🟡 在线扩容 PVC
kubectl patch pvc mysql-data -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 🟢 查看卷挂载信息
kubectl get volumeattachment
mount | grep csi
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| PVC Pending | StorageClass 不存在/容量不足 | `kubectl describe pvc` | 检查 SC/扩容存储池 |
| Pod 挂载卷超时 | CSI 插件异常/节点网络 | `kubectl logs csi-rbdplugin-*` | 重启 CSI Pod/检查网络 |
| 卷扩容失败 | 存储后端不支持/文件系统未扩展 | `kubectl describe pvc` | 检查 allowVolumeExpansion/resize2fs |
| 快照失败 | Snapshotter 未部署/权限不足 | `kubectl get volumesnapshotclass` | 安装 snapshot-controller |
| Ceph HEALTH_WARN | OSD down/PG degraded | `ceph status`; `ceph health detail` | 检查 OSD/等待 PG 恢复 |

### 排查流程

```
存储异常排查
├── PVC Pending？
│   ├── kubectl describe pvc → Events 查看原因
│   ├── StorageClass 存在？→ kubectl get sc
│   └── 存储后端容量？→ ceph df / 云控制台
├── 卷挂载失败？
│   ├── CSI Pod 运行？→ kubectl get pods -n rook-ceph
│   ├── 节点可达？→ 检查网络/存储网络
│   └── 卷已被其他节点挂载？→ RWO 限制
└── 数据丢失/损坏？
    ├── 检查 reclaimPolicy（Delete/Retain）
    ├── 检查 VolumeSnapshot 是否可用
    └── 检查 Velero 备份是否可用
```

## 生产案例

### 案例1：Ceph OSD 磁盘故障导致 PG 降级

- **场景**：一块 SSD 故障导致 30 个 PG 变为 degraded，IO 延迟飙升
- **排查**：`ceph health detail` 显示 OSD.15 down；`ceph osd tree` 确认磁盘故障
- **方案**：标记 OSD out → 更换磁盘 → 重建 OSD → 等待 PG 恢复
- **效果**：10分钟内 IO 恢复正常（副本读取），2小时后 PG 全部 active+clean

### 案例2：PVC 扩容后文件系统未扩展

- **场景**：PVC 从 50Gi 扩容到 100Gi，但 Pod 内 df 仍显示 50G
- **排查**：`kubectl describe pvc` 显示 FileSystemResizePending；需要 Pod 重启触发 resize
- **方案**：滚动重启 Pod 触发 kubelet 执行 resize2fs；确认 StorageClass 支持 allowVolumeExpansion
- **效果**：Pod 重启后文件系统自动扩展到 100G

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| Rook-Ceph | 功能全面、块/文件/对象、自修复 | 运维复杂、资源开销大 | 大规模生产集群 |
| Longhorn | 简单、UI友好、轻量 | 性能稍弱、扩展性有限 | 中小规模/边缘 |
| 云存储 (EBS/Azure Disk) | 托管、高可用、无需运维 | 厂商锁定、成本较高 | 公有云环境 |
| NFS | 简单、ReadWriteMany | 单点故障、性能瓶颈 | 开发/测试环境 |
| OpenEBS | 轻量、多种引擎 | 社区较小、功能较少 | 轻量级存储需求 |

## 检查清单

- [ ] StorageClass 已创建且设置为默认（或明确指定）
- [ ] CSI 驱动 Pod 在所有存储节点运行
- [ ] VolumeSnapshotClass 已配置（如需快照）
- [ ] Velero 备份已配置且定期验证恢复
- [ ] PVC reclaimPolicy 设置为 Retain（生产数据）
- [ ] 存储容量监控告警已配置（>80% 告警）
- [ ] 多副本/多 AZ 存储高可用已配置
- [ ] 存储扩容流程已验证

## Related

- [[实体/k8s-control-plane-deep-dive.md|k8s-control-plane-deep-dive]] — 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[概念/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]


<!-- risk-assessed -->
