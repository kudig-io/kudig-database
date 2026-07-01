---
title: 卷
description: Volume 是 Kubernetes 中为 Pod 容器提供文件系统访问的存储抽象。容器内的文件默认是临时的，Volume 解决了数据持久化和容器间共享存储的...
summary: Volume 是 Kubernetes 中为 Pod 容器提供文件系统访问的存储抽象。容器内的文件默认是临时的，Volume 解决了数据持久化和容器间共享存储的...
category: dictionary
tags:
- k8s
- glossary
- storage
- volume
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 卷 是什么
- Volume 详解
trigger_keywords:
- 卷
- Volume
- dictionary
prerequisites:
- kubectl-basics
---



# 卷

> **英文名**: Volume

## 概述

Volume 是 Kubernetes 中为 Pod 容器提供文件系统访问的存储抽象。容器内的文件默认是临时的，Volume 解决了数据持久化和容器间共享存储的需求。

## 核心概念/原理

### Volume 类型

Kubernetes 支持多种 Volume 类型：

- **emptyDir**：临时目录，Pod 删除后数据丢失。
- **hostPath**：挂载节点文件系统的特定路径。
- **configMap/secret**：将 ConfigMap/Secret 作为文件挂载。
- **persistentVolumeClaim**：挂载持久化存储。
- **projected**：将多个 Volume 源合并为一个目录。
- **csi**：直接使用 CSI 驱动提供的卷。
- **nfs**：挂载 NFS 共享。

## 关键机制或特性

- Volume 的生命周期与 Pod 绑定（emptyDir）或独立于 Pod（PV）。
- 容器崩溃（非 Pod 删除）时，emptyDir 数据不丢失。
- Volume 可以以只读或读写方式挂载。

## 使用场景与最佳实践

- 临时数据使用 emptyDir。
- 配置文件使用 ConfigMap Volume。
- 敏感信息使用 Secret Volume。
- 持久化数据使用 PVC。
- 避免使用 hostPath（安全风险和可移植性问题）。

## 参考链接

- [Volume - Official Documentation](https://kubernetes.io/docs/concepts/storage/volumes/)

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/emptydir.md|Emptydir]]
- [[domain-17-system-foundation/topic-dictionary/storage/hostpath.md|Hostpath]]
