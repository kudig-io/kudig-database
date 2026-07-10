---
title: 持久化卷
description: PersistentVolume（PV）是 Kubernetes 中集群级别的存储资源，由管理员预先创建或通过 StorageClass
  动态供给。它代表集群中...
summary: PersistentVolume（PV）是 Kubernetes 中集群级别的存储资源，由管理员预先创建或通过 StorageClass 动态供给。它代表集群中...
category: dictionary
tags:
- k8s
- glossary
- storage
- pv
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 持久化卷 是什么
- PersistentVolume (PV) 详解
trigger_keywords:
- 持久化卷
- PersistentVolume (PV)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 持久化卷

> **英文名**: PersistentVolume (PV)

## 概述

PersistentVolume（PV）是 Kubernetes 中集群级别的存储资源，由管理员预先创建或通过 StorageClass 动态供给。它代表集群中的一块实际存储（如云磁盘、NFS 共享等），独立于 Pod 的生命周期。

## 核心概念/原理

### 核心概念

- **PV 属性**：容量（capacity）、访问模式（access modes）、回收策略（reclaim policy）、存储类。
- **访问模式**：
  - `ReadWriteOnce (RWO)`：单节点读写。
  - `ReadOnlyMany (ROX)`：多节点只读。
  - `ReadWriteMany (RWX)`：多节点读写。
  - `ReadWriteOncePod (RWOP)`：单 Pod 读写（v1.22+）。
- **回收策略**：`Retain`（保留数据）、`Delete`（删除存储）、`Recycle`（已弃用）。

### PV 生命周期

```
Available → Bound → Released → (Available/Delete)
```

## 关键机制或特性

- PV 与 PVC 是一对一绑定关系。
- `persistentVolumeReclaimPolicy: Retain` 确保数据不会被意外删除。
- 静态供给需要管理员预先创建 PV 对象。

## 使用场景与最佳实践

- 生产环境优先使用动态供给（StorageClass）。
- 为关键数据使用 `Retain` 回收策略。
- 监控 PV 的状态（Available/Bound/Released）。

## 参考链接

- [PersistentVolume (PV) - Official Documentation](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)

## Related

- [[系统基础/知识字典/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[系统基础/知识字典/storage/storage-class.md|Storage Class]]
- [[系统基础/知识字典/storage/emptydir.md|Emptydir]]
- [[系统基础/知识字典/storage/hostpath.md|Hostpath]]
- [[系统基础/知识字典/configuration/configmap.md|Configmap]]


<!-- risk-assessed -->
