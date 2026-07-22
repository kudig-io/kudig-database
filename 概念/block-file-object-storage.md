---
title: Block, File, and Object Storage
description: '- [[概念/storage-tool-evolution.md|storage-tool-evolution]] — 存储工具演进'
summary: '- [[概念/storage-tool-evolution.md|storage-tool-evolution]] — 存储工具演进'
category: concepts
tags:
- k8s
- storage
- block
- file
- object
- csi
- ceph
- minio
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Block, File, and Object Storage 是什么
- 如何 Block, File, and Object Storage
trigger_keywords:
- Block
- File
- and
- Object
- Storage
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Block, File, and Object Storage

## Storage Type Comparison

| Property | Block Storage | File Storage | Object Storage |
|----------|--------------|-------------|----------------|
| Data Unit | Fixed-size blocks (512B-4KB) | Files and directories | Objects (data + metadata) |
| Access Method | Block device interface | File path (POSIX) | HTTP API (S3-compatible) |
| Filesystem | Client formats | Server-managed | Not applicable |
| Performance | Highest IOPS, lowest latency | Moderate | Lower per-op, high throughput |
| Sharing | Single-node (typically) | Multi-node concurrent | Multi-node via API |
| Protocols | SATA/SAS, FC, iSCSI, NVMe-oF | NFS, SMB, CephFS | S3, Swift |
| K8s AccessMode | ReadWriteOnce | ReadWriteMany | Via CSI or sidecar |

## Block Storage

Protocols from fastest to most accessible:
- **NVMe-oF**: Ultra-low latency over RDMA or TCP (newest, highest performance)
- **Fibre Channel**: Dedicated SAN fabric, very low latency
- **iSCSI**: SCSI over IP network, widely used enterprise SAN
- **SATA/SAS**: Direct-attached storage

K8s maps block storage via `ReadWriteOnce` PersistentVolumes (e.g., AWS EBS, GCE PD, Ceph RBD).

## File Storage

File storage provides POSIX-compatible shared access:
- **NFSv4**: Stateful protocol with security enhancements, most common in K8s
- **SMB/CIFS**: Windows file sharing, used in mixed environments
- **CephFS**: Distributed POSIX filesystem on Ceph RADOS layer

K8s uses `ReadWriteMany` access mode for file storage, enabling multiple [[Pods|Pods]] to share data.

## Object Storage

Object storage provides flat, HTTP-accessible data storage with massive scalability:
- **S3 API**: Industry standard (Amazon S3, MinIO, Ceph RGW)
- **Key-based addressing**: Each object has a unique key, no directory hierarchy
- **Metadata-rich**: Custom metadata per object

Common K8s use cases: backup archives, static assets, data lakes, ML training data. Tools like MinIO provide S3-compatible object storage on K8s.

## RAID Redundancy

| RAID | Min Disks | Capacity | Fault Tolerance |
|------|-----------|----------|----------------|
| RAID 0 | 2 | 100% | None |
| RAID 1 | 2 | 50% | Single disk |
| RAID 5 | 3 | (n-1)/n | Single disk |
| RAID 6 | 4 | (n-2)/n | Dual disk |
| RAID 10 | 4 | 50% | Multiple disks |

## K8s Storage Integration

K8s CSI (Container Storage Interface) abstracts storage providers. Block storage: CSI drivers provision RWO volumes. File storage: CSI drivers provision RWX volumes (NFS, CephFS). Object storage: No native CSI; accessed via S3 SDK or s3fs/goofys mount.

## 源码实现分析

### CSI 驱动架构

```
┌─────────────────────────────────────────────────┐
│  kubelet (VolumeManager)                       │
│  └── CSI Plugin (in-tree)                      │
│      ├── NodeGetInfo → 获取节点拓扑          │
│      ├── NodeStageVolume → 格式化+挂载到全局  │
│      └── NodePublishVolume → bind mount 到 Pod │
└─────────────────────────────────────────────────┘
         │ gRPC (unix socket)
         ▼
┌─────────────────────────────────────────────────┐
│  CSI Driver (DaemonSet - Node Plugin)          │
│  └── 实际执行: mount/umount/mkfs              │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│  CSI Driver (StatefulSet - Controller Plugin)  │
│  ├── CreateVolume → 调用存储后端 API         │
│  ├── DeleteVolume → 删除卷                    │
│  ├── ControllerPublishVolume → Attach 到节点  │
│  └── CreateSnapshot → 创建快照              │
└─────────────────────────────────────────────────┘
```

### 存储类型与 K8s 映射

| 存储类型 | 典型后端 | K8s AccessMode | CSI 操作 | 适用场景 |
|----------|----------|---------------|----------|----------|
| 块存储 | EBS/Ceph RBD | RWO | CreateVolume + Attach + Mount | 数据库、有状态服务 |
| 文件存储 | NFS/CephFS/EFS | RWX | CreateVolume + Mount | 共享文件、媒体 |
| 对象存储 | S3/MinIO/RGW | 无原生支持 | 通过 SDK/sidecar | 备份、数据湖、ML |

## 使用场景

### 场景一：数据库块存储（高性能 IOPS）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: high-iops-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: io2
  iopsPerGB: "50"       # 每 GB 50 IOPS
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
spec:
  accessModes: [ReadWriteOnce]  # 块存储: 单节点
  storageClassName: high-iops-ssd
  resources:
    requests:
      storage: 200Gi
```

### 场景二：对象存储访问（Sidecar 模式）

```yaml
# K8s 无原生对象存储 CSI，通过 sidecar 挂载
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    image: myapp:1.0
    volumeMounts:
    - name: s3-data
      mountPath: /data/s3
  - name: s3fs
    image: s3fs:latest
    args: ["my-bucket", "/data/s3"]
    env:
    - name: AWS_ACCESS_KEY_ID
      valueFrom:
        secretKeyRef:
          name: s3-creds
          key: access-key
    securityContext:
      privileged: true    # s3fs 需要 FUSE
    volumeMounts:
    - name: s3-data
      mountPath: /data/s3
      mountPropagation: Bidirectional
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 块存储可以多 Pod 共享 | 块存储 RWO 只能单节点挂载，多 Pod 共享需文件存储 |
| 对象存储可以用 PV 挂载 | K8s 无原生对象存储 CSI，需 SDK/sidecar/s3fs |
| NFS 性能足够数据库使用 | NFS 延迟高、无 POSIX 锁保证，数据库应用块存储 |
| 所有云盘都支持扩容 | 需 StorageClass 设置 allowVolumeExpansion: true |
| RAID 可以替代分布式存储副本 | RAID 只防磁盘故障，不防节点故障，分布式副本两者都防 |
| CephFS 和 Ceph RBD 一样 | RBD 是块存储（RWO），CephFS 是文件存储（RWX），底层都是 RADOS |

## 面试要点

1. **块/文件/对象存储如何选择？** — 数据库/有状态服务→块存储（高 IOPS、低延迟）；多 Pod 共享文件→文件存储（RWX）；备份/归档/ML 训练数据→对象存储（海量、低成本、HTTP 访问）。

2. **CSI 驱动的工作原理？** — Controller Plugin（StatefulSet）：CreateVolume/DeleteVolume/Attach；Node Plugin（DaemonSet）：Stage(格式化+全局挂载)/Publish(bind mount 到 Pod)。通过 gRPC 与 kubelet CSI Plugin 交互。

3. **Ceph 存储架构？** — RADOS（对象存储层）→ RBD（块设备接口）→ CephFS（POSIX 文件系统）→ RGW（S3/Swift 对象网关）。底层统一用 CRUSH 算法分布数据，无中心元数据服务器。

4. **生产环境存储容量规划？** — 监控 PVC 使用率（kubelet volume stats）；设置告警（80% 预警）；启用自动扩容（allowVolumeExpansion）；定期清理快照；对象存储用生命周期策略自动转冷/删除。

## Related

- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Kubernetes Core Concepts
- [[概念/storage-tool-evolution.md|storage-tool-evolution]] — 存储工具演进
- [[概念/overlayfs-storage.md|overlayfs-storage]] — OverlayFS Storage
- [[概念/linux-sysctl-tuning.md|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[实体/csi-drivers.md|csi-drivers]] — CSI Drivers
- [[概念/linux-sysctl-tuning.md|Linux Sysctl Tuning]]
- Container Storage Interface

- 02-block-file-object-storage

<!-- risk-assessed -->
