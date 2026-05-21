---
title: Block, File, and Object Storage
description: '- [[concepts/storage-tool-evolution.md|storage-tool-evolution]] — 存储工具演进'
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

K8s uses `ReadWriteMany` access mode for file storage, enabling multiple Pods to share data.

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

- [[concepts/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Kubernetes Core Concepts
- [[concepts/storage-tool-evolution.md|storage-tool-evolution]] — 存储工具演进
- [[concepts/overlayfs-storage.md|overlayfs-storage]] — OverlayFS Storage
- [[concepts/linux-sysctl-tuning.md|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[entities/csi-drivers.md|csi-drivers]] — CSI Drivers
- [[concepts/linux-sysctl-tuning.md|Linux Sysctl Tuning]]
- Container Storage Interface

- [[domain-04-storage-data/02-block-file-object-storage.md|02-block-file-object-storage]]