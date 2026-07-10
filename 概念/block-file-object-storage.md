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
