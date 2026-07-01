---
title: Storage Capacity（存储容量）
description: '# Storage Capacity（存储容量）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Storage Capacity（存储容量） 是什么
- 如何 Storage Capacity（存储容量）
trigger_keywords:
- Storage
- Capacity
- 存储容量
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Storage Capacity（存储容量）

## 概述

存储容量跟踪是 [[Kubernetes|Kubernetes]] 在 v1.24 达到稳定（stable）的一项功能。它使 Kubernetes 能够跟踪集群中各节点的可用存储容量，并在调度 Pod 时将其作为考量因素，从而减少因节点存储不足导致的调度失败和重试。

## 核心概念/原理

- **拓扑感知**：网络存储可能无法被所有节点访问，本地存储更是与特定节点绑定。调度器需要知道每个节点可访问的存储容量。
- **容量信息来源**：CSI 驱动通过创建 `CSIStorageCapacity` 对象报告各拓扑下的存储容量。
- **调度决策**：调度器在放置 Pod 时，检查节点是否有足够的容量来创建尚未存在的卷。

## 关键机制或特性

### API 扩展

1. **CSIStorageCapacity 对象**
   - 由 CSI 驱动在驱动安装的命名空间中创建。
   - 每个对象包含一种 StorageClass 在特定拓扑（一组节点）下的容量信息。

2. **CSIDriverSpec.StorageCapacity**
   - 当设置为 `true` 时，Kubernetes 调度器在为使用该 CSI 驱动的卷做调度决策时会考虑存储容量。

### 调度行为

调度器仅在以下情况使用存储容量信息：

- Pod 使用了尚未创建的卷；
- 该卷使用的 StorageClass 引用了某个 CSI 驱动，且绑定模式为 `WaitForFirstConsumer`；
- 对应 `CSIDriver` 对象的 `StorageCapacity` 字段为 `true`。

此时，调度器只会将 Pod 调度到具有足够容量的节点上。该检查比较简单，仅比较卷大小与包含该节点拓扑的 `CSIStorageCapacity` 对象中的容量值。

### 重新调度（Rescheduling）

- 由于调度器使用的容量信息可能已过时，实际创建卷时可能失败。
- 如果卷创建失败，节点选择会被重置，调度器会重新尝试为 Pod 寻找合适的节点。

### 限制

- 存储容量跟踪能提高首次调度成功的概率，但无法完全保证，因为容量信息可能不是实时的。
- 如果一个 Pod 使用多个卷，可能出现第一个卷已在某个拓扑段创建成功，但剩余容量不足以创建第二个卷的情况，此时需要人工干预（如扩容或删除已创建卷）。
- CSI 临时卷（CSI [[domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes.md|ephemeral volumes]]es（卷）|volumes]]）的调度**不**考虑存储容量。

## 使用场景

- **本地存储调度**：使用本地 PV 或拓扑受限的 CSI 存储时，确保 Pod 被调度到有足够空间的节点上。
- **大规模集群**：在节点众多、存储后端复杂的集群中，减少因存储不足导致的调度重试次数，提升调度效率。
- **多 StorageClass 环境**：不同 StorageClass 对应不同存储池，调度器根据各池容量做出合理调度决策。

## 最佳实践/注意事项

- 确保所使用的 CSI 驱动支持并启用了存储容量跟踪功能。
- 对于 `Immediate` 绑定模式的卷，存储驱动自行决定卷的创建位置，调度器不参与容量检查。
- 为 `WaitForFirstConsumer` 模式的 StorageClass 配置容量跟踪，能显著改善 Pod 调度体验。
- 当 Pod 使用多个卷时，需关注可能出现部分卷创建成功但后续卷容量不足的情况。

## 生产 YAML 示例

### 启用存储容量跟踪的 CSI 驱动

```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: local.csi.example.com
spec:
  storageCapacity: true                    # 启用容量跟踪
  volumeLifecycleModes:
    - Persistent
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 被调度到容量不足的节点 | 容量信息过期 | `kubectl get csistoragecapacity` 检查更新时间 |
| 多卷 Pod 部分创建成功部分失败 | 第一个卷消耗完剩余容量 | 需人工干预：扩容或删除已创建卷 |
| CSIStorageCapacity 对象不存在 | CSI 驱动未启用容量跟踪 | 确认 CSIDriver 的 `storageCapacity: true` |

## 生产检查清单

- [ ] 本地存储 / 拓扑受限 CSI 驱动启用 `storageCapacity: true`
- [ ] 配合 `WaitForFirstConsumer` 绑定模式
- [ ] 监控 CSIStorageCapacity 对象的更新频率

## 命令快速参考

```bash
# 查看存储容量对象
kubectl get csistoragecapacity -A

# 查看特定 SC 的容量
kubectl get csistoragecapacity -A -o custom-columns='SC:.storageClassName,CAPACITY:.capacity,NODES:.nodeTopology'
```

## 交叉引用

- [存储类](./storage-classes.md) — WaitForFirstConsumer 绑定模式
- [节点特定卷限制](./[[domain-17-system-foundation/topic-dictionary/storage/node-specific-volume-limits.md|node-specific-volume-limits]].md) — 节点级卷数量限制
- [动态卷供给](./dynamic-volume-provisioning.md) — 容量感知供给

## 参考链接

- https://kubernetes.io/docs/concepts/storage/storage-capacity/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[domain-17-system-foundation/topic-dictionary/storage/composefs.md|ComposeFS 只读文件系统]]
