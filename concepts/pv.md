---
title: "PersistentVolume (PV)"
category: concepts
tags: ["storage", "pv", "persistent-volume", "core", "visibility/public"]
sources: ["concepts/"]
created: 2026-05-24
updated: 2026-05-24
status: stub
---
# PersistentVolume (PV)

## 概述

PersistentVolume（PV）是 Kubernetes 集群中的一块存储资源，由管理员预先配置或通过 StorageClass 动态供给。PV 独立于使用它的 Pod 生命周期，用于为应用提供持久化存储能力。

## 远程顾问诊断要点

- 询问用户 PV 的供给方式（静态/动态）
- 检查 StorageClass 配置和 provisioner 状态
- 确认 PV 的 ReclaimPolicy（Retain/Recycle/Delete）

## 相关链接

- [[concepts/persistent-volume-claim|PersistentVolumeClaim]] — PVC 声明与绑定
- [[concepts/storageclass|StorageClass]] — 存储类动态供给
- [[concepts/kubernetes|Kubernetes]] — 核心概念
