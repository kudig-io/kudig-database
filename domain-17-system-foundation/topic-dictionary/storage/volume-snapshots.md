---
title: Volume Snapshots（卷快照）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- crd
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volume Snapshots（卷快照） 是什么
- 如何 Volume Snapshots（卷快照）
trigger_keywords:
- Volume
- Snapshots
- 卷快照
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---



# Volume Snapshots（卷快照）

## 概述

在 [[Kubernetes|Kubernetes]] 中，VolumeSnapshot 表示对存储系统上某个卷在特定时间点的快照。卷快照为用户提供了一种标准化的方式，用于在不创建全新卷的情况下复制卷的内容。此功能对于数据库备份、灾难恢复和数据迁移等场景非常重要。

## 核心概念/原理

- **VolumeSnapshot**：用户请求创建快照的资源，类似于 PersistentVolumeClaim。
- **VolumeSnapshotContent**：集群中实际快照资源的表示，由存储系统上的真实快照支持，类似于 PersistentVolume。
- **VolumeSnapshotClass**：定义快照的属性（如存储提供商特定的参数），类似于 StorageClass。
- **CRD 资源**：VolumeSnapshot、VolumeSnapshotContent 和 VolumeSnapshotClass 都是自定义资源定义（CRD），不属于核心 API。
- **仅支持 CSI**：卷快照功能仅适用于实现了 CSI 快照接口的 CSI 驱动。

## 关键机制或特性

### 生命周期

1. **预创建（Pre-provisioned）**：管理员手动创建 VolumeSnapshotContent，用户创建 VolumeSnapshot 并引用它。
2. **动态创建（Dynamic provisioning）**：用户创建 VolumeSnapshot 并指定数据源 PVC，系统根据 VolumeSnapshotClass 自动创建底层快照和 VolumeSnapshotContent。

### 绑定与保护

- 快照控制器负责将 VolumeSnapshot 与 VolumeSnapshotContent 一对一绑定。
- **使用保护（In-Use Protection）**：当正在对某个 PVC 拍摄快照时，该 PVC 被保护，不能被立即删除；删除操作会延迟到快照变为 `readyToUse` 或被中止。

### DeletionPolicy（删除策略）

- **Delete**：删除 VolumeSnapshot 时，同时删除底层存储快照和 VolumeSnapshotContent。
- **Retain**：删除 VolumeSnapshot 时，保留底层存储快照和 VolumeSnapshotContent。

### 从快照恢复卷

- 新建 PVC 时可通过 `dataSource` 字段引用 VolumeSnapshot，从而创建一个预填充了快照数据的新卷：
  ```yaml
  dataSource:
    name: new-snapshot-test
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  ```

### 卷模式保护（SourceVolumeMode）

- `sourceVolumeMode` 字段记录源卷的 `Filesystem` 或 `Block` 模式。
- 默认情况下，系统会阻止用户从快照创建与源卷模式不同的 PVC。
- 如需允许模式变更，可在 VolumeSnapshotContent 上添加注解 `snapshot.storage.kubernetes.io/allow-volume-mode-change: "true"`。

## 使用场景

- **数据备份**：在数据库编辑或删除操作前创建快照，以便在出现问题时快速恢复。
- **快速数据克隆**：通过快照创建新的预填充卷，用于开发测试环境的快速搭建。
- **灾难恢复**：定期创建生产卷快照，保存到异地存储以应对灾难场景。

## 最佳实践/注意事项

- 使用卷快照前，必须确保集群已部署快照控制器（snapshot controller）和 CSI 驱动的 `csi-snapshotter` sidecar。
- 快照仅支持 CSI 驱动；in-tree 插件不支持此功能。
- 删除 VolumeSnapshot 前请确认其 `deletionPolicy`，避免误删底层存储快照。
- 对于预创建的快照，管理员需要正确设置 `snapshotHandle`（存储后端快照 ID）和 `sourceVolumeMode`。

## 生产 YAML 示例

### 创建卷快照 + 从快照恢复

```yaml
# 1. 创建 VolumeSnapshot
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: postgres-snap-20260407
  namespace: database
spec:
  volumeSnapshotClassName: csi-ebs-snapclass
  source:
    persistentVolumeClaimName: postgres-data
---
# 2. 从快照恢复新 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-restored
  namespace: database
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: gp3-encrypted
  resources:
    requests:
      storage: 100Gi                       # >= 源卷大小
  dataSource:
    name: postgres-snap-20260407
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| VolumeSnapshot 状态 readyToUse: false | 底层快照创建中或失败 | `kubectl describe volumesnapshot`；检查 snapshot-controller 日志 |
| 从快照恢复 PVC Pending | VolumeSnapshotContent 不存在或未绑定 | `kubectl get volumesnapshotcontent` |
| 快照删除后底层快照未清理 | deletionPolicy 为 Retain | 手动清理底层存储快照 |

## 生产检查清单

- [ ] 部署 snapshot-controller 和 CSI snapshotter sidecar
- [ ] 为生产数据使用 `deletionPolicy: Retain`
- [ ] 定期创建快照并验证恢复流程
- [ ] 设置快照保留策略（自动清理过期快照）

## 命令快速参考

```bash
# 查看快照
kubectl get volumesnapshots -n database

# 查看快照详情
kubectl describe volumesnapshot postgres-snap-20260407 -n database

# 查看 VolumeSnapshotContent
kubectl get volumesnapshotcontent
```

## 交叉引用

- [卷快照类](./volume-snapshot-classes.md) — VolumeSnapshotClass 配置
- [持久卷](./persistent-volumes.md) — PVC dataSource 恢复
- [CSI 卷克隆](./csi-volume-cloning.md) — 另一种数据复制方式

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-snapshots/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[domain-17-system-foundation/topic-dictionary/storage/composefs.md|ComposeFS 只读文件系统]]
