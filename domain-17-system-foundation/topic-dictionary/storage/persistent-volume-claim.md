---
title: 持久化卷声明
description: 'PersistentVolumeClaim（PVC）是用户对存储资源的请求。类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。用户通过 PVC 指定所需的...'
category: dictionary
tags:
- k8s
- glossary
- storage
- pvc
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
created: "2026-06-24"
---

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

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume.md|Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/emptydir.md|Emptydir]]
- [[domain-17-system-foundation/topic-dictionary/storage/hostpath.md|Hostpath]]
