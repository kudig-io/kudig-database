---
title: 持久化卷声明
description: PersistentVolumeClaim（PVC）是用户对存储资源的请求。类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。用户通过
  PVC 指定所需的...
summary: PersistentVolumeClaim（PVC）是用户对存储资源的请求。类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。用户通过 PVC
  指定所需的...
category: dictionary
tags:
- k8s
- glossary
- storage
- pvc
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 持久化卷声明 是什么
- PersistentVolumeClaim (PVC) 详解
trigger_keywords:
- 持久化卷声明
- PersistentVolumeClaim (PVC)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 持久化卷声明

> **英文名**: PersistentVolumeClaim (PVC)

## 概述

PersistentVolumeClaim（PVC）是用户对存储资源的请求。类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。用户通过 PVC 指定所需的存储大小和访问模式。

## 核心概念/原理

### 核心概念

- **PVC 请求参数**：存储容量、访问模式、StorageClass。
- **绑定过程**：PVC 与满足条件的 PV 自动绑定（静态供给）或通过 StorageClass 动态创建 PV（动态供给）。
- **使用方式**：PVC 作为 Volume 挂载到 Pod 中。

### PVC 示例

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: standard
  resources:
    requests:
      storage: 10Gi
```

## 关键机制或特性

- PVC 创建后可以扩容（如果 StorageClass 的 `allowVolumeExpansion: true`）。
- PVC 不能缩减容量。
- 使用 `volumeMode: Block` 获取原始块设备而非文件系统。

## 使用场景与最佳实践

- 根据应用需求选择合适的访问模式和 StorageClass。
- 为关键应用使用 ReadWriteOnce + Retain 策略。
- 定期检查 PVC 的状态和使用率。

## 参考链接

- [PersistentVolumeClaim (PVC) - Official Documentation](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)

## Related

- [[系统基础/知识字典/storage/persistent-volume.md|Persistent Volume]]
- [[系统基础/知识字典/storage/storage-class.md|Storage Class]]
- [[系统基础/知识字典/storage/volume.md|Volume]]
- [[系统基础/知识字典/storage/emptydir.md|Emptydir]]
- [[系统基础/知识字典/storage/hostpath.md|Hostpath]]


<!-- risk-assessed -->
