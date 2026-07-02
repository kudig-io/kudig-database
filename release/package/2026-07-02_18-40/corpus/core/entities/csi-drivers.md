---
title: CSI Drivers
description: CSI Drivers — Kubernetes 生产运维知识库
summary: CSI Drivers — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- csi
- storage
- plugin
- volume
- provisioning
- ceph
- statefulset
- daemonset
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
- CSI Drivers 是什么
- 如何 CSI Drivers
trigger_keywords:
- CSI
- Drivers
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CSI Drivers

## CSI Architecture

CSI defines a standard gRPC interface between Kubernetes and storage vendors:

| Component | Runs As | Responsibilities |
|-----------|---------|-----------------|
| **CSI Controller** | Centralized Deployment | CreateVolume, DeleteVolume, ControllerPublish/UnpublishVolume, CreateSnapshot |
| **CSI Node** | [[DaemonSet|DaemonSet]] on every node | NodeStage/UnstageVolume, NodePublish/UnpublishVolume, NodeGetCapabilities |

## Popular CSI Drivers

| Driver | Backend | Features |
|--------|---------|----------|
| **AWS EBS CSI** | Amazon EBS volumes | gp3/io1/io2, encryption, snapshots |
| **Azure Disk CSI** | Azure Managed Disks | Premium/Standard SSD, encryption |
| **GCE PD CSI** | Google Persistent Disk | pd-standard/pd-ssd, snapshots |
| **Ceph CSI** | Ceph RBD/CephFS | Distributed storage, RWX support |
| **NFS CSI** | NFS servers | File sharing, RWX |

## CSI Migration

Kubernetes has migrated all in-tree volume plugins to CSI. The migration was gradual:
1. In-tree plugins coexist with CSI drivers
2. Feature gates enable CSI migration per plugin
3. In-tree plugins are deprecated and will be removed

## Volume Lifecycle

1. **Provision**: CSI Controller creates volume on storage backend
2. **Attach**: CSI Controller attaches volume to node (block storage only)
3. **Mount**: CSI Node plugin mounts volume to Pod's filesystem
4. **Unmount**: On Pod deletion, CSI Node plugin unmounts
5. **Detach**: CSI Controller detaches from node
6. **Delete**: CSI Controller destroys volume (if reclaimPolicy=Delete)

## Related

- [[grpc]] — gRPC
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/storage-model.md|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[skills/manage-persistent-storage.md|manage-persistent-storage]] — Manage Persistent Storage
- [[concepts/storage-model.md|Persistent Storage Model]]
- [[skills/manage-persistent-storage.md|Manage Persistent Storage]]
- [[entities/statefulset.md|StatefulSet]]

- 05-csi-drivers-integration

<!-- risk-assessed -->
