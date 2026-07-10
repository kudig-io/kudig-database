---
title: Persistent Storage Model (PV/PVC/StorageClass)
description: '- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
summary: '- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
category: concepts
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- csi
- volumes
- ceph
- statefulset
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
- Persistent Storage Model (PV/PVC/StorageClass) 是什么
- 如何 Persistent Storage Model (PV/PVC/StorageClass)
trigger_keywords:
- Persistent
- Storage
- Model
- PV
- PVC
- StorageClass
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Persistent Storage Model (PV/PVC/StorageClass)

## Three-Way Storage Abstraction

Kubernetes decouples storage from workloads through three objects:

1. **StorageClass**: A provisioning template defining how volumes are created (driver, parameters, reclaim policy, volume expansion support). Enables dynamic provisioning.

2. **PersistentVolume (PV)**: A cluster-scoped volume resource with capacity, access mode, reclaim policy, and storage class. Can be statically pre-provisioned or dynamically created.

3. **PersistentVolumeClaim (PVC)**: A namespace-scoped request for storage. The PVC Controller binds PVCs to matching PVs based on storage class, access mode, and size.

## Access Modes

| Mode | Description | Use Case |
|------|-------------|----------|
| **RWO** (ReadWriteOnce) | Mounted by one node | Most databases, block storage |
| **ROX** (ReadOnlyMany) | Read-only by multiple nodes | Shared data, reference data |
| **RWX** (ReadWriteMany) | Read-write by multiple nodes | Shared file systems, media |

## CSI (Container Storage Interface)

CSI is the standard plugin interface for storage vendors. It provides:
- **Controller [[Service|Service]]**: Create/Delete/Attach/Detach volumes
- **Node Service**: Stage/Unstage/Publish/Unpublish volumes on nodes

Popular CSI drivers include AWS EBS CSI, Azure Disk CSI, Ceph CSI, and cloud provider-specific drivers.

## Reclaim Policies

| Policy | Behavior |
|--------|----------|
| **Retain** | Volume preserved after PVC deletion; requires manual cleanup |
| **Delete** | Volume and data destroyed when PVC deleted |
| **Recycle** | Data scrubbed, volume returned to pool (deprecated) |

## Storage Best Practices

- Use StorageClass with `volumeBindingMode: WaitForFirstConsumer` to schedule Pods before provisioning volumes (avoids zone mismatch)
- Enable volume expansion for databases that need to grow
- Use appropriate access modes (RWO for databases, RWX for shared file storage)

## Related
- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合

- [[kanister]] — Kanister
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[技能/manage-persistent-storage.md|manage-persistent-storage]] — Manage Persistent Storage
- [[实体/csi-drivers.md|csi-drivers]] — CSI Drivers
- [[实体/csi-drivers.md|CSI Drivers]]
- [[实体/statefulset.md|StatefulSet]]
- [[技能/manage-persistent-storage.md|Manage Persistent Storage]]
- [[概念/resource-management.md|Resource Management]]

- [[概念/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]

<!-- risk-assessed -->
