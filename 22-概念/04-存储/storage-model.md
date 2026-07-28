---
title: Persistent Storage Model (PV/PVC/StorageClass)
description: '- [[22-概念/11-交叉分析/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
summary: '- [[22-概念/11-交叉分析/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
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
status: reviewed
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
- **Controller [[service|Service]]**: Create/Delete/Attach/Detach volumes
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

## 源码实现分析

### PV Controller 绑定流程

kube-controller-manager 中的 PersistentVolumeController 负责 PVC 与 PV 的绑定：

```go
// kubernetes/pkg/controller/volume/persistentvolume/pv_controller.go
func (ctrl *PersistentVolumeController) syncClaim(ctx context.Context, claim *v1.PersistentVolumeClaim) error {
    // 1. 检查 PVC 是否已绑定
    if claim.Status.Phase == v1.ClaimBound {
        return ctrl.syncBoundClaim(ctx, claim)  // 验证 PV 仍存在
    }
    // 2. 查找匹配的 PV（静态绑定）
    volume, err := ctrl.findBestMatchForClaim(claim)
    if volume != nil {
        return ctrl.bind(ctx, volume, claim)   // 直接绑定
    }
    // 3. 触发动态供给（通过 StorageClass provisioner）
    if class := util.GetPersistentVolumeClaimClass(claim); class != "" {
        ctrl.provisionClaim(ctx, claim)  // 调用 CSI CreateVolume
    }
    return nil
}
```

### CSI 动态供给调用链

```
PVC 创建 (Pending)
    │
    ▼
PV Controller → external-provisioner (sidecar)
    │
    ▼
CSI CreateVolume RPC → Storage Backend (Ceph/EBS/NFS)
    │
    ▼
PV 对象创建 → PVC Bound
    │
    ▼
Pod 调度 → kubelet VolumeManager
    │
    ▼
CSI NodeStageVolume + NodePublishVolume → 挂载到 Pod
```

## 使用场景

### 场景一：数据库动态供给（WaitForFirstConsumer）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer  # 等 Pod 调度后再创建卷
allowVolumeExpansion: true
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 100Gi
```

### 场景二：共享文件存储（RWX）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: shared-nfs
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs.internal.example.com
  share: /exports/shared
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: media-storage
spec:
  accessModes: [ReadWriteMany]  # 多 Pod 同时读写
  storageClassName: shared-nfs
  resources:
    requests:
      storage: 500Gi
```

### 场景三：在线扩容

```bash
# 🟡 中风险 - 修改 PVC 大小（仅支持扩容，不可缩容）
kubectl patch pvc postgres-data -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 🟢 低风险 - 确认扩容状态
kubectl get pvc postgres-data -o jsonpath='{.status.conditions}'
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| PV 删除后数据自动清除 | 取决于 reclaimPolicy：Retain 保留数据，Delete 才清除 |
| RWO 表示只能一个 Pod 挂载 | RWO 是单“节点”挂载，同节点多 Pod 可共享同一 PV |
| PVC 创建即绑定 PV | WaitForFirstConsumer 模式下，需等 Pod 调度后才绑定 |
| 任何 PV 都可以扩容 | 需 StorageClass 设置 allowVolumeExpansion: true |
| RWX 所有存储都支持 | 块存储（EBS/Ceph RBD）不支持 RWX，仅文件存储支持 |
| PVC 缩小可以释放空间 | K8s 不支持 PVC 缩容，需迁移数据到新 PVC |

## 面试要点

1. **PV/PVC/StorageClass 三者关系？** — StorageClass 定义“如何创建卷”（provisioner + 参数），PVC 是用户对存储的“申请单”，PV 是实际的存储资源。动态供给时 PVC 触发 StorageClass 的 provisioner 自动创建 PV。

2. **WaitForFirstConsumer 解决什么问题？** — 解决可用区不匹配问题。若 PVC 立即绑定，PV 可能在 zone-a 创建，但 Pod 被调度到 zone-b，导致挂载失败。WaitForFirstConsumer 延迟绑定直到 Pod 调度完成。

3. **CSI 驱动的工作原理？** — CSI 通过 gRPC 接口与 kubelet 交互：Controller Service（CreateVolume/DeleteVolume/Attach）运行在 StatefulSet 中，Node Service（Stage/Publish）运行在 DaemonSet 中。external-provisioner/external-attacher 作为 sidecar 监听 PVC/VolumeAttachment 变化并调用 CSI。

4. **生产环境存储选型考虑？** — 数据库用 RWO + SSD（gp3/ceph-rbd）；共享文件用 RWX（CephFS/NFS/EFS）；日志/临时数据用 emptyDir 或 local PV；备份用 Velero + 对象存储。关键指标：IOPS、延迟、吐量、扩容能力、快照支持。

## Related
- [[22-概念/11-交叉分析/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合

- [[kanister]] — Kanister
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/07-调度与资源/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[26-技能/06-存储/csi-storage/manage-persistent-storage.md|manage-persistent-storage]] — Manage Persistent Storage
- [[23-实体/02-K8s核心组件/csi-drivers.md|csi-drivers]] — CSI Drivers
- [[23-实体/02-K8s核心组件/csi-drivers.md|CSI Drivers]]
- [[23-实体/02-K8s核心组件/statefulset.md|StatefulSet]]
- [[26-技能/06-存储/csi-storage/manage-persistent-storage.md|Manage Persistent Storage]]
- [[22-概念/07-调度与资源/resource-management.md|Resource Management]]

- [[22-概念/11-交叉分析/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]

<!-- risk-assessed -->
