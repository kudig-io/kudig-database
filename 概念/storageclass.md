---
title: StorageClass
summary: StorageClass 是 Kubernetes 中用于定义存储"类"的资源对象，它描述了存储卷的质量-of-service 级别、备份策略或集群管理员定义的任意策略。通过
  StorageClass，Kubernetes 可以实现存储的动态供给（Dynamic Provisioning）。
category: concepts
tags:
- storage
- storageclass
- dynamic-provisioning
- core
- visibility/public
tier: core
sources:
- concepts/
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
status: stub
---


# StorageClass

## 概述

StorageClass 是 Kubernetes 中用于定义存储"类"的资源对象，它描述了存储卷的质量-of-service 级别、备份策略或集群管理员定义的任意策略。通过 StorageClass，Kubernetes 可以实现存储的动态供给（Dynamic Provisioning）。

## 远程顾问诊断要点

- 询问用户使用的 provisioner 类型（如 alicloud-disk、csi-plugin）
- 检查 StorageClass 的 VolumeBindingMode（Immediate/WaitForFirstConsumer）
- 确认 reclaimPolicy 和 allowVolumeExpansion 设置

## 相关链接

- [[概念/pv.md|PersistentVolume]] — 持久化卷
- [[概念/persistent-volume-claim.md|PersistentVolumeClaim]] — 持久化卷声明
- [[概念/kubernetes.md|Kubernetes]] — 核心概念

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
