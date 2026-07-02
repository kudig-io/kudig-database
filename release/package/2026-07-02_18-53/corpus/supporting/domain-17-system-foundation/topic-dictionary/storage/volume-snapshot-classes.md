---
title: Volume Snapshot Classes（卷快照类）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volume Snapshot Classes（卷快照类） 是什么
- 如何 Volume Snapshot Classes（卷快照类）
trigger_keywords:
- Volume
- Snapshot
- Classes
- 卷快照类
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Volume Snapshot Classes（卷快照类）

## 概述

VolumeSnapshotClass 与 StorageClass 类似，它提供了一种由管理员描述快照“类别”的机制。当动态创建卷快照时，VolumeSnapshotClass 定义了使用哪个 CSI 驱动、删除策略以及特定于存储提供商的参数。

## 核心概念/原理

- **快照类别配置**：每个 VolumeSnapshotClass 包含 `driver`、`deletionPolicy` 和 `parameters`，用于动态创建 VolumeSnapshotContent。
- **不可变对象**：VolumeSnapshotClass 一旦创建，其字段不能被更新。
- **按驱动选择默认类**：与 StorageClass 不同，可以为每个 CSI 驱动分别设置一个默认 VolumeSnapshotClass。

## 关键机制或特性

### 主要字段

| 字段 | 说明 |
|------|------|
| `driver` | 指定用于创建快照的 CSI 驱动名称。 |
| `deletionPolicy` | 删除策略：`Delete`（删除底层快照）或 `Retain`（保留底层快照）。 |
| `parameters` | 传递给 CSI 驱动的键值对参数，驱动特定。 |

### 默认 VolumeSnapshotClass

- 通过注解 `snapshot.storage.[[Kubernetes|kubernetes]].io/is-default-class: "true"` 标记默认类。
- 当创建 VolumeSnapshot 未指定 `volumeSnapshotClassName` 时，Kubernetes 会自动选择一个默认 VolumeSnapshotClass。
- **匹配规则**：系统会根据 PVC 的 StorageClass 所使用的 CSI 驱动，选择具有相同 CSI `driver` 的默认 VolumeSnapshotClass。
- 每个 CSI 驱动应只配置一个默认 VolumeSnapshotClass；如果同一驱动存在多个默认类，创建快照将失败。

### 删除策略

- **Delete**：删除 VolumeSnapshot 时，自动删除对应的 VolumeSnapshotContent 和底层存储快照。
- **Retain**：删除 VolumeSnapshot 时，保留 VolumeSnapshotContent 和底层存储快照，便于后续恢复或审计。

### 参数

- VolumeSnapshotClass 的 `parameters` 用于描述属于该类的快照特性，具体可接受的参数取决于 `driver`。
- 示例参数可能包括快照类型、存储层、压缩策略等，需参考具体 CSI 驱动的文档。

## 使用场景

- **不同保留策略的快照分类**：为生产数据创建 `deletionPolicy: Retain` 的快照类，为临时测试数据创建 `deletionPolicy: Delete` 的快照类。
- **多存储后端快照管理**：集群中使用多个 CSI 驱动时，为每个驱动配置独立的 VolumeSnapshotClass。
- **自动化快照创建**：配合备份工具，利用默认 VolumeSnapshotClass 实现无需指定类的自动快照创建。

## 最佳实践/注意事项

- 确保每个 CSI 驱动最多只有一个默认 VolumeSnapshotClass，避免快照创建时因无法选择而失败。
- 根据数据的重要性和合规要求选择合适的 `deletionPolicy`。
- VolumeSnapshotClass 创建后不可更新；如需更改配置，需删除旧类并创建新类。
- 在创建 VolumeSnapshot 之前，确认目标 CSI 驱动已正确安装并支持快照功能。

## 生产 YAML 示例

### VolumeSnapshotClass（Retain + Delete）

```yaml
# 生产环境 — 保留快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-ebs-snapclass-retain
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"
driver: ebs.csi.aws.com
deletionPolicy: Retain
parameters:
  tagSpecification_1: "Environment=production"
---
# 开发测试 — 自动删除
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-ebs-snapclass-delete
driver: ebs.csi.aws.com
deletionPolicy: Delete
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 创建快照失败，提示无默认 VolumeSnapshotClass | 未配置默认类 | 添加 `is-default-class: "true"` 注解 |
| 同一 CSI 驱动有多个默认类 | 每个驱动只允许一个默认类 | `kubectl get volumesnapshotclass` 检查注解 |

## 生产检查清单

- [ ] 每个 CSI 驱动最多一个默认 VolumeSnapshotClass
- [ ] 生产数据使用 `deletionPolicy: Retain`
- [ ] 测试环境使用 `deletionPolicy: Delete` 避免快照堆积

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VolumeSnapshotClass
kubectl get volumesnapshotclass

# 查看详情
kubectl describe volumesnapshotclass csi-ebs-snapclass-retain
```
## 交叉引用

- [卷快照](./volume-snapshots.md) — VolumeSnapshot 创建与恢复
- [存储类](./storage-classes.md) — StorageClass 与 VolumeSnapshotClass 类比

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-snapshot-classes/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[domain-17-system-foundation/topic-dictionary/storage/composefs.md|ComposeFS 只读文件系统]]


<!-- risk-assessed -->
