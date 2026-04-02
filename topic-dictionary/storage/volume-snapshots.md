# Volume Snapshots（卷快照）

## 概述

在 Kubernetes 中，VolumeSnapshot 表示对存储系统上某个卷在特定时间点的快照。卷快照为用户提供了一种标准化的方式，用于在不创建全新卷的情况下复制卷的内容。此功能对于数据库备份、灾难恢复和数据迁移等场景非常重要。

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

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-snapshots/
