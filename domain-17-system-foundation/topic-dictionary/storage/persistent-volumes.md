---
title: Persistent Volumes（持久卷）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- mysql
- postgresql
- kafka
- statefulset
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Persistent Volumes（持久卷） 是什么
- 如何 Persistent Volumes（持久卷）
trigger_keywords:
- Persistent
- Volumes
- 持久卷
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- kafka-basics
- mysql-basics
created: "2026-05-23"
---

# Persistent Volumes（持久卷）

## 概述

PersistentVolume（PV）和 PersistentVolumeClaim（PVC）是 [[Kubernetes|Kubernetes]] 中用于抽象存储供给与消费的 API 资源。PV 代表集群中的一块存储，由管理员预先创建或通过 StorageClass 动态供给；PVC 是用户对存储的请求，类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。

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

## 生产 YAML 示例

### PVC + StorageClass 动态供给

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
  namespace: database
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: gp3-encrypted          # 引用 StorageClass
  resources:
    requests:
      storage: 100Gi
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: database
spec:
  serviceName: postgres
  replicas: 1
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
        - name: postgres
          image: postgres:16
          volumeMounts:
            - name: data
              mountPath: /var/lib/postgresql/data
          resources:
            requests:
              cpu: "1"
              memory: 2Gi
      volumes:
        - name: data
          persistentVolumeClaim:
            claimName: postgres-data
```

### 从快照恢复 PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-restored
  namespace: database
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: gp3-encrypted
  resources:
    requests:
      storage: 100Gi
  dataSource:
    name: postgres-snapshot-20260407
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| PVC 一直 Pending | 无匹配 PV 或 StorageClass provisioner 异常 | `kubectl describe pvc` 查看 Events；检查 CSI 驱动 Pod 日志 |
| PV 状态为 Released 但无法被新 PVC 绑定 | Retain 策略下 PV 需要手动清理 | 删除 PV 的 `claimRef`：`kubectl patch pv <name> -p '{"spec":{"claimRef":null}}'` |
| 卷扩展后容器内大小未变 | 文件系统未扩展 | 检查 PVC conditions `FileSystemResizePending`；确认 CSI 支持在线扩展 |
| Pod 被删除但 PV 未释放 | Storage Object in Use Protection | PVC 被 Pod 引用时有 Finalizer 保护；等待 Pod 终止 |

## 生产检查清单

- [ ] 使用动态供给 + StorageClass，避免手动创建 PV
- [ ] 关键数据 PV 使用 `reclaimPolicy: Retain`
- [ ] StorageClass 启用 `allowVolumeExpansion: true`
- [ ] 拓扑受限存储使用 `volumeBindingMode: WaitForFirstConsumer`
- [ ] 需要严格单 Pod 访问时使用 RWOP 访问模式
- [ ] 迁移 in-tree 插件到 CSI 驱动

## 命令快速参考

```bash
# 查看 PV/PVC 状态
kubectl get pv,pvc -n database

# 查看 PVC 详情
kubectl describe pvc postgres-data -n database

# 扩展 PVC
kubectl patch pvc postgres-data -n database -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 清理 Released PV 的 claimRef
kubectl patch pv <pv-name> -p '{"spec":{"claimRef":null}}'
```

## 交叉引用

- [存储类](./storage-classes.md) — StorageClass 定义与 provisioner 配置
- [卷快照](./volume-snapshots.md) — 从快照恢复 PVC
- [CSI 卷克隆](./csi-volume-cloning.md) — 从现有 PVC 克隆
- [动态卷供给](./dynamic-volume-provisioning.md) — 动态供给机制
- [卷](./volumes.md) — 卷类型总览

## 参考链接

- https://kubernetes.io/docs/concepts/storage/persistent-volumes/

## Related

- index/backup-dr-index|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
