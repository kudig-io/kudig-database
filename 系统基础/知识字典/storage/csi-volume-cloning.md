---
title: CSI Volume Cloning（CSI 卷克隆）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
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
- CSI Volume Cloning（CSI 卷克隆） 是什么
- 如何 CSI Volume Cloning（CSI 卷克隆）
trigger_keywords:
- CSI
- Volume
- Cloning
- 卷克隆
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CSI Volume Cloning（CSI 卷克隆）

## 概述

CSI 卷克隆功能允许用户在创建新的 PersistentVolumeClaim（PVC）时，通过引用一个已有的 PVC 作为数据源，让后端存储系统创建一个与源卷内容完全相同的副本。克隆在功能上与普通卷相同，唯一的区别是在供给时不会创建空卷，而是复制已有卷的数据。

## 核心概念/原理

- **克隆定义**：克隆是现有 [[Kubernetes|Kubernetes]] 卷的一个精确副本，可以像标准卷一样被消费。
- **数据源引用**：在新建 PVC 的 `dataSource` 字段中指定源 PVC 的名称和类型（`PersistentVolumeClaim`）。
- **后端实现**：实际的克隆操作由底层 CSI 驱动在存储后端执行，而非 Kubernetes 本身复制数据。

## 关键机制或特性

### 使用条件

- **仅支持 CSI 驱动**：卷克隆功能仅适用于支持克隆能力的 CSI 驱动。
- **源 PVC 要求**：源 PVC 必须处于已绑定（Bound）状态且未被使用（not in use）。
- **同一命名空间**：源 PVC 和目标 PVC 必须位于同一命名空间。
- **容量要求**：新 PVC 的 `spec.resources.requests.storage` 必须等于或大于源卷的容量。

### 示例配置

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: clone-of-pvc-1
  namespace: myns
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: cloning
  resources:
    requests:
      storage: 5Gi
  dataSource:
    kind: PersistentVolumeClaim
    name: pvc-1
```

### 独立性

- 克隆完成后，新 PVC 是一个完全独立的对象。
- 它可以被单独使用、再次克隆、拍摄快照或删除。
- 源 PVC 与克隆 PVC 之间没有任何链接关系；修改或删除源 PVC 不会影响克隆。

## 使用场景

- **快速环境复制**：为开发或测试环境快速复制生产数据库的数据卷。
- **大数据处理**：在不影响原始数据的情况下，为分析任务创建数据副本。
- **有状态应用迁移**：通过克隆创建数据副本，再将其挂载到新的 Pod 或集群中。

## 最佳实践/注意事项

- 在发起克隆前，确保源 PVC 已被绑定且未被任何 Pod 使用。
- 克隆的目标 PVC 容量不能小于源 PVC 的容量。
- 克隆操作的速度和效率取决于底层存储系统的实现，不同 CSI 驱动的性能可能有显著差异。
- 源 PVC 和目标 PVC 必须在同一命名空间中；跨命名空间克隆需借助其他机制（如快照恢复）。

## 生产 YAML 示例

### 从生产 PVC 克隆到测试环境

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data-clone
  namespace: staging
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: gp3-encrypted
  resources:
    requests:
      storage: 100Gi                       # >= 源 PVC 容量
  dataSource:
    kind: PersistentVolumeClaim
    name: db-data                          # 源 PVC（同命名空间）
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 克隆 PVC Pending | 源 PVC 未绑定或正在使用 | `kubectl get pvc` 检查源 PVC 状态 |
| 容量不足错误 | 目标 PVC storage < 源 PVC | 设置目标 PVC storage >= 源 PVC |
| CSI 驱动不支持 | 驱动未实现克隆能力 | 查阅 CSI 驱动文档确认克隆支持 |

## 生产检查清单

- [ ] 源 PVC 已绑定且未被使用
- [ ] 目标 PVC storage >= 源 PVC
- [ ] 源和目标 PVC 在同一命名空间
- [ ] CSI 驱动支持克隆能力

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建克隆 PVC
kubectl apply -f clone-pvc.yaml

# 查看克隆进度
kubectl describe pvc db-data-clone -n staging
```
## 交叉引用

- [卷快照](./volume-snapshots.md) — 另一种数据复制方式（跨命名空间可用）
- [持久卷](./persistent-volumes.md) — dataSource 机制
- [动态卷供给](./dynamic-volume-provisioning.md) — 克隆依赖动态供给

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-pvc-datasource/

## Related

- [[系统基础/知识字典/storage/ceph.md|Ceph]]
- [[系统基础/知识字典/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[系统基础/知识字典/storage/composefs.md|ComposeFS 只读文件系统]]


<!-- risk-assessed -->
