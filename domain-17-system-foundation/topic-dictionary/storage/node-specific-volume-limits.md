---
title: Node-specific Volume Limits（节点特定卷限制）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- statefulset
- gpu
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Node-specific Volume Limits（节点特定卷限制） 是什么
- 如何 Node-specific Volume Limits（节点特定卷限制）
trigger_keywords:
- Node-specific
- Volume
- Limits
- 节点特定卷限制
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
created: "2026-05-23"
created: 2026-05
---

# Node-specific Volume Limits（节点特定卷限制）

## 概述

[[Kubernetes|Kubernetes]] 需要尊重每个节点可以附加（attach）的卷数量上限。云厂商（如 AWS、GCP、Azure）通常对每块虚拟机可挂载的磁盘数量有限制。如果不遵守这些限制，调度到该节点的 Pod 可能会因卷无法附加而卡在等待状态。

## 核心概念/原理

- **卷数量上限**：不同云厂商和实例类型支持附加的卷数量不同。
- **调度约束**：Kubernetes 调度器在将 Pod 分配到节点时，会检查该节点当前已附加的卷数量是否已达上限。
- **动态检测**：Kubernetes 能够根据节点类型和 CSI 驱动报告的信息动态确定卷限制。

## 关键机制或特性

### Kubernetes 默认限制

| 云服务 | 每节点最大卷数 |
|--------|----------------|
| Amazon EBS | 39 |
| Google Persistent Disk | 16 |
| Microsoft Azure Disk | 16 |

### 动态卷限制（Dynamic Volume Limits）

Kubernetes v1.17 [stable]

支持的卷类型：
- Amazon EBS
- Google Persistent Disk
- Azure Disk
- CSI

对于 in-tree 卷插件，Kubernetes 自动识别节点类型并强制执行对应的最大卷数：
- **GCE**：最多 127 个卷（取决于节点类型）。
- **AWS EBS**：M5、C5、R5、T3、Z1D 实例类型限制为 25 个卷；其他实例类型限制为 39 个卷。
- **Azure Disk**：最多 64 个磁盘（取决于节点类型和大小）。

对于 CSI 驱动：
- 如果 CSI 驱动在 `NodeGetInfo` 中报告了最大卷数，调度器会尊重该限制。
- 已迁移到 CSI 的 in-tree 插件，其限制以 CSI 驱动报告的值为准。

### 可变 CSI 节点可分配计数（Mutable CSI Node Allocatable Count）

Kubernetes v1.35 [beta]（默认启用）

- CSI 驱动可以在运行时动态调整节点可附加的最大卷数。
- 通过在 `CSIDriver` 规格中设置 `nodeAllocatableUpdatePeriodSeconds`，[[kubelet|kubelet]] 会定期调用 `NodeGetInfo` 刷新限制。
- 最小允许间隔为 10 秒。
- 如果卷附加操作因资源耗尽（`ResourceExhausted`，[[gRPC|gRPC]] code 8）而失败，Kubernetes 会立即更新该节点的可分配卷数，并将受影响的 Pod 标记为 Failed，防止无限卡在 `ContainerCreating` 状态。

### 防止在未安装 CSI 驱动的节点上放置 Pod

Kubernetes v1.35 [alpha]（默认禁用）

- 启用 `VolumeLimitScaling` 特性门后，如果某个 CSI 驱动已安装对应的 `CSIDriver` 对象，调度器会阻止需要该 CSI 卷的 Pod 被调度到尚未安装该驱动的节点上。

## 使用场景

- **高密度存储节点**：在大规模有状态应用中，确保节点不会因为挂载过多卷而超出云平台限制。
- **异构实例类型集群**：混合使用不同规格实例的集群中，自动根据实例类型应用正确的卷数量上限。
- **动态资源变化**：节点上其他资源（如 GPU、网络接口）占用 attachment 槽位时，CSI 驱动可动态下调可挂载卷数。

## 最佳实践/注意事项

- 设计有状态应用时，应考虑节点卷限制，避免单节点运行过多需要独立卷的 Pod。
- 如果使用 CSI 驱动，确保其实现了 `NodeGetInfo` 中的最大卷数报告，以便调度器正确决策。
- 对于已迁移到 CSI 的 in-tree 插件，动态卷限制以 CSI 驱动为准，无需额外配置。
- 在 v1.35+ 集群中，`MutableCSINodeAllocatableCount` 默认启用，CSI 驱动开发者可利用此特性提高调度准确性。

## 生产 YAML 示例

### 查看节点卷限制

```bash
# 查看节点的可附加卷上限
kubectl get csinode <node-name> -o yaml

# 查看节点 Allocatable 中的 attachable-volumes
kubectl describe node <node-name> | grep attachable-volumes
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 卡在 ContainerCreating，提示 volume limit reached | 节点已附加卷数达上限 | `kubectl describe node` 查看 `attachable-volumes-*` |
| 新 CSI 驱动的限制为 0 | 驱动未在 NodeGetInfo 报告限制 | 检查 CSI 驱动实现 |
| 限制值与预期不符 | in-tree 迁移到 CSI 后限制以 CSI 为准 | `kubectl get csinode -o yaml` 查看报告的限制 |

## 生产检查清单

- [ ] 规划节点卷密度：每节点最大 Pod 数 × 每 Pod 卷数 <= 节点卷限制
- [ ] 大规模 [[StatefulSet|StatefulSet]] 注意分散到足够多节点
- [ ] CSI 驱动正确实现 NodeGetInfo 报告最大卷数

## 命令快速参考

```bash
# 查看各节点的卷限制
kubectl get nodes -o custom-columns='NAME:.metadata.name,VOLUMES:.status.allocatable.attachable-volumes-csi-ebs\.csi\.aws\.com'

# 查看 CSINode 信息
kubectl get csinodes -o wide
```

## 交叉引用

- [存储容量](./storage-capacity.md) — 容量维度的调度约束
- [持久卷](./persistent-volumes.md) — PV 附加到节点
- [存储类](./storage-classes.md) — 不同存储后端的卷限制差异

## 参考链接

- https://kubernetes.io/docs/concepts/storage/storage-limits/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[domain-17-system-foundation/topic-dictionary/storage/composefs.md|ComposeFS 只读文件系统]]
