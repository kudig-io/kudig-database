---
title: 存储类
description: 'StorageClass 是 Kubernetes 中定义存储类别的资源。它使管理员能够描述不同质量级别的存储（如 SSD/HDD、性能等级），并实现存储的动态...'
category: dictionary
tags:
- k8s
- glossary
- storage
- storageclass
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储类 是什么
- StorageClass 详解
trigger_keywords:
- 存储类
- StorageClass
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 存储类

> **英文名**: StorageClass

## 概述

StorageClass 是 Kubernetes 中定义存储类别的资源。它使管理员能够描述不同质量级别的存储（如 SSD/HDD、性能等级），并实现存储的动态供给。

## 核心概念/原理

### 核心属性

- **provisioner**：指定使用哪个 CSI 驱动或内置供给器创建存储。
- **parameters**：传递给存储供给器的特定参数（如磁盘类型、IOPS）。
- **reclaimPolicy**：动态创建的 PV 的回收策略。
- **allowVolumeExpansion**：是否允许 PVC 扩容。
- **volumeBindingMode**：`Immediate`（立即绑定）或 `WaitForFirstConsumer`（等待 Pod 调度后绑定）。

### 示例

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

## 关键机制或特性

- `WaitForFirstConsumer` 模式避免创建存储时不知道 Pod 会调度到哪个节点的问题。
- 可以设置默认 StorageClass（通过注解 `storageclass.kubernetes.io/is-default-class: "true"`）。
- CSI 驱动的 StorageClass 比 in-tree 的更灵活。

## 使用场景与最佳实践

- 为不同工作负载定义不同级别的 StorageClass（如 fast-ssd、standard-hdd）。
- 生产环境使用 `WaitForFirstConsumer` 绑定模式。
- 启用 `allowVolumeExpansion` 以支持在线扩容。

## 参考链接

- [StorageClass - Official Documentation](https://kubernetes.io/docs/concepts/storage/storage-classes/)

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume.md|Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/emptydir.md|Emptydir]]
- [[domain-17-system-foundation/topic-dictionary/storage/hostpath.md|Hostpath]]
