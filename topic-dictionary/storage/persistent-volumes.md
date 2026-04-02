# Persistent Volumes（持久卷）

## 概述

PersistentVolume（PV）和 PersistentVolumeClaim（PVC）是 Kubernetes 中用于抽象存储供给与消费的 API 资源。PV 代表集群中的一块存储，由管理员预先创建或通过 StorageClass 动态供给；PVC 是用户对存储的请求，类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。

## 核心概念/原理

- **PersistentVolume（PV）**：集群级别的存储资源，生命周期独立于 Pod。它可以是 NFS、iSCSI、云厂商存储或本地存储等。
- **PersistentVolumeClaim（PVC）**：命名空间级别的资源，用户通过 PVC 请求特定大小和访问模式的存储，无需关心底层实现细节。
- **StorageClass**：定义存储的“类别”，用于区分不同性能、备份策略等存储选项，是动态供给的基础。

### 生命周期

1. **Provisioning（供给）**：静态（管理员手动创建 PV）或动态（根据 StorageClass 自动创建 PV）。
2. **Binding（绑定）**：控制平面将 PVC 与匹配的 PV 一对一绑定。
3. **Using（使用）**：Pod 通过 `persistentVolumeClaim` 卷类型挂载 PVC，从而使用底层 PV。
4. **Reclaiming（回收）**：PVC 删除后，根据 PV 的回收策略处理底层存储：
   - **Retain**：保留数据和 PV，需要管理员手动清理。
   - **Delete**：删除 PV 及底层存储资产（动态供给的默认策略）。
   - **Recycle**（已弃用）：执行基本清理后重新可用。

## 关键机制或特性

### 访问模式（Access Modes）

| 模式 | 说明 |
|------|------|
| `ReadWriteOnce` (RWO) | 可被单个节点以读写方式挂载，允许同一节点上的多个 Pod 访问。 |
| `ReadOnlyMany` (ROX) | 可被多个节点以只读方式挂载。 |
| `ReadWriteMany` (RWX) | 可被多个节点以读写方式挂载。 |
| `ReadWriteOncePod` (RWOP) | 仅允许单个 Pod 在整个集群中以读写方式挂载（v1.29 stable）。 |

### Volume Mode

- `Filesystem`（默认）：卷被格式化为文件系统并挂载到目录。
- `Block`：卷作为原始块设备提供给 Pod，适用于需要直接操作块设备的应用。

### 存储对象使用保护（Storage Object in Use Protection）

- 正在使用的 PVC 或被 PVC 绑定的 PV 在被删除时不会立即移除，而是通过 Finalizer 延迟删除，防止数据丢失。

### 卷扩展（Volume Expansion）

- 支持通过修改 PVC 的 `resources.requests.storage` 字段来扩展卷大小（仅支持增大，不支持缩小）。
- 需要 StorageClass 的 `allowVolumeExpansion: true`，且底层驱动支持扩展。
- 支持在线扩展（in-use PVC 无需删除 Pod）。

### 数据源与卷填充器（Data Source & Volume Populators）

- 新建 PVC 时可通过 `dataSource` 或 `dataSourceRef` 从 VolumeSnapshot、现有 PVC（克隆）或自定义资源预填充数据。
- `dataSourceRef` 支持引用更多类型的对象（需开启 `AnyVolumeDataSource` 特性门）。

### 跨命名空间数据源

- 通过 `CrossNamespaceVolumeDataSource`（Alpha）和 ReferenceGrant，允许 PVC 引用其他命名空间中的数据源。

## 使用场景

- **有状态应用**：数据库（MySQL、PostgreSQL）、消息队列（Kafka）等需要持久化数据的场景。
- **数据共享**：多个 Pod 需要共享只读数据（如模型文件、静态资源），可使用 ROX/RWX 模式的 PV。
- **数据备份与恢复**：通过 Snapshot 或克隆创建 PVC，实现数据的快速恢复和迁移。

## 最佳实践/注意事项

- 编写可移植配置时，只包含 PVC，不包含 PV；由集群自动处理动态供给。
- 不直接编辑 PV 的容量，应该通过修改 PVC 来触发自动扩展。
- 对于本地存储，建议设置 StorageClass 的 `volumeBindingMode: WaitForFirstConsumer`，确保调度器能综合考虑 Pod 的约束条件。
- 注意不同存储插件对访问模式的支持差异，选择合适的访问模式。
- 尽量使用 CSI 驱动，避免依赖已弃用或移除的 in-tree 插件。

## 参考链接

- https://kubernetes.io/docs/concepts/storage/persistent-volumes/
