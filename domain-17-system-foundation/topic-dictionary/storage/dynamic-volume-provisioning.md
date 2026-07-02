---
title: Dynamic Volume Provisioning（动态卷供给）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- statefulset
- rag
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dynamic Volume Provisioning（动态卷供给） 是什么
- 如何 Dynamic Volume Provisioning（动态卷供给）
trigger_keywords:
- Dynamic
- Volume
- Provisioning
- 动态卷供给
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Dynamic Volume Provisioning（动态卷供给）

## 概述

动态卷供给允许存储卷在需要时按需创建。没有动态供给时，集群管理员必须手动联系云或存储提供商创建新存储，然后再在 [[Kubernetes|Kubernetes]] 中创建 PersistentVolume 对象来表示它们。动态供给消除了这一繁琐过程，当用户创建 PersistentVolumeClaim（PVC）时，系统会自动为其创建相应的存储卷。

## 核心概念/原理

- **基于 StorageClass**：动态供给的实现依赖于 `storage.k8s.io` API 组中的 StorageClass 对象。
- **按需创建**：用户创建 PVC 时，Kubernetes 根据 PVC 中指定的 StorageClass 自动调用相应的 provisioner 创建 PV 和底层存储。
- **解耦用户与管理员**：管理员预先配置好 StorageClass，用户只需在 PVC 中引用即可，无需了解底层存储细节。

## 关键机制或特性

### 启用动态供给

1. 集群管理员预先创建一个或多个 StorageClass，每个类指定：
   - `provisioner`：使用哪个卷插件/CSI 驱动。
   - `parameters`：传递给 provisioner 的参数。
2. 用户在 PVC 中通过 `storageClassName` 字段指定所需的 StorageClass。
3. 控制平面检测到 PVC 后，调用 provisioner 动态创建 PV 和底层存储。

### 默认 StorageClass

- 通过将 StorageClass 标记为默认（`storageclass.kubernetes.io/is-default-class: "true"`），用户创建未指定 `storageClassName` 的 PVC 时，会自动使用该默认类。
- 需要确保 API 服务器启用了 `DefaultStorageClass` 准入控制器。
- 如果存在多个默认 StorageClass，Kubernetes 会选择最新创建的那个。

### 历史变更

- Kubernetes v1.6 之前，通过注解 `volume.beta.kubernetes.io/storage-class` 指定存储类。
- 从 v1.9 开始，推荐使用 PVC 规格中的 `storageClassName` 字段，旧注解已弃用。

### 多区域集群中的动态供给

- 在跨可用区的集群中，单可用区存储后端应在 Pod 调度的可用区中创建。
- 通过设置 StorageClass 的 `volumeBindingMode: WaitForFirstConsumer`，可以确保存储在 Pod 被调度后再创建，从而匹配 Pod 所在的拓扑位置。

## 使用场景

- **自助式存储申请**：开发团队可以直接创建 PVC 获取存储，无需等待管理员手动配置。
- **弹性伸缩**：配合 [[StatefulSet|StatefulSet]]、Deployment 等控制器，实现有状态应用的自动存储扩展。
- **多存储后端混合使用**：同一集群中可同时配置多个 StorageClass，分别对接 SSD、HDD、NFS、对象存储等不同后端。

## 最佳实践/注意事项

- 始终使用 `storageClassName` 字段而非旧注解来指定 StorageClass。
- 为集群配置一个明确的默认 StorageClass，避免用户因遗漏 `storageClassName` 而导致 PVC 无法绑定。
- 对于拓扑受限的存储（如云盘、本地存储），使用 `WaitForFirstConsumer` 绑定模式，防止 Pod 调度失败。
- 确保集群中已安装并正确配置了对应 StorageClass 的 CSI 驱动或外部 provisioner。

## 生产 YAML 示例

### 动态供给完整流程

```yaml
# 1. StorageClass（管理员创建）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  encrypted: "true"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
---
# 2. PVC（用户创建 — 触发动态供给）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data
  namespace: production
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 50Gi
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| PVC Pending 无 provisioning 事件 | StorageClass provisioner 不存在 | `kubectl describe pvc`；确认 CSI 驱动已安装 |
| 默认 SC 未生效 | DefaultStorageClass 准入控制器未启用 | 检查 apiserver 的 `--enable-admission-plugins` |
| 多区域集群 PV 创建在错误区域 | 使用 Immediate 绑定模式 | 改用 `WaitForFirstConsumer` |

## 生产检查清单

- [ ] 配置一个明确的默认 StorageClass
- [ ] 使用 `storageClassName` 字段（非旧注解）
- [ ] 拓扑受限存储使用 `WaitForFirstConsumer`
- [ ] 确认 CSI 驱动已正确安装

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 StorageClass
kubectl get sc

# 查看 PVC 的 provisioning 事件
kubectl describe pvc app-data -n production

# 查看 CSI 驱动 Pod
kubectl get pods -n kube-system -l app.kubernetes.io/name=aws-ebs-csi-driver
```
## 交叉引用

- [存储类](./storage-classes.md) — StorageClass 定义与参数
- [持久卷](./persistent-volumes.md) — PV/PVC 生命周期
- [存储容量](./storage-capacity.md) — 容量感知动态供给

## 参考链接

- https://kubernetes.io/docs/concepts/storage/dynamic-provisioning/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume.md|Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/emptydir.md|Emptydir]]


<!-- risk-assessed -->
