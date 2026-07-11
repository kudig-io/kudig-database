---
title: 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
description: '# 存储体系'
summary: '# 存储体系'
category: reference
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- csi
- backup
- etcd
- scheduler
- ceph
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复 是什么
- 如何 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
trigger_keywords:
- 存储体系：PV
- PVC
- StorageClass
- CSI
- 驱动与灾备恢复
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 存储体系

> **CNCF 状态**: 生态概览 | **类别**: Storage | **主要语言**: YAML

## 概述

Kubernetes 存储生态系统是一个涵盖 CSI 驱动、存储类、卷快照、数据保护等多层面的综合技术体系。它定义了容器存储接口（CSI）标准，让存储厂商可以通过统一的插件接口为 K8s 提供持久化存储能力。该生态系统包括 CSI 驱动（如 Ceph RBD、NFS、EBS、Azure Disk）、存储编排工具（如 Rook、Longhorn）、数据保护方案（如 Velero、Kasten）等多个组件。

## Key Features（核心能力）

- **CSI 标准**：Container Storage Interface 统一了存储提供商的接入方式
- **StorageClass**：动态 PV 供应，支持存储分层和 QoS
- **VolumeSnapshot**：卷快照和恢复机制
- **Volume Expansion**：在线卷扩容
- **Rook/Longhorn**：K8s 原生的分布式存储编排
- **Velero**：集群资源和 PV 数据的备份恢复

## 架构与工作原理

K8s 存储生态由多个层级构成：存储介质层（块存储、文件存储、对象存储）；CSI 驱动层（Provisioner、Attacher、Snapshotter 三组件）；K8s API 层（PV/PVC/StorageClass/VolumeSnapshot CRD）；编排管理层（Rook operator、Longhorn manager）。PVC 通过 StorageClass 动态创建 PV，CSI 驱动与底层存储系统交互完成实际卷操作。

## K8s 集成

K8s 存储核心概念包括 PersistentVolume（PV，集群级存储资源）、PersistentVolumeClaim（PVC，用户级存储请求）、StorageClass（动态供应策略）。CSI 驱动通过 Sidecar 组件（external-provisioner、external-attacher、external-snapshotter）与 K8s 控制平面交互。Pod 通过 volumeMounts 引用 PVC，kubelet 通过 CSI gRPC 接口挂载卷到 Pod。

## 生产用例

- **数据库持久化**：MySQL/PostgreSQL 的持久化存储
- **消息队列存储**：Kafka/RabbitMQ 的数据卷
- **数据备份恢复**：Velero 定期备份 PV 数据到 S3
- **多区域存储**：跨 AZ 的存储复制和高可用

## 安装与快速开始

```bash
# 安装 Rook-Ceph 分布式存储
helm repo add rook-release https://charts.rook.io/release
helm install rook-ceph rook-release/rook-ceph -n rook-ceph --create-namespace
# 创建 StorageClass
kubectl apply -f https://raw.githubusercontent.com/rook/rook/master/deploy/examples/csi/rbd/storageclass.yaml
```

## 对比替代方案

相比 in-tree 存储插件（已废弃），CSI 提供了标准化、可插拔的存储接口。相比外部存储阵列，Rook/Longhorn 提供 K8s 原生的分布式存储。

## Related

- [[实体/k8s-control-plane-deep-dive.md|k8s-control-plane-deep-dive]] — 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[概念/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]


<!-- risk-assessed -->
