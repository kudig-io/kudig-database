---
title: Carina (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- storage
- carina
- scheduler
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Carina 是什么
- 如何 Carina
trigger_keywords:
- Carina
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Carina

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

Carina 是一个 Kubernetes 本地存储供应器，基于 LVM（Logical Volume Manager）管理节点上的本地磁盘，为有状态应用提供高性能的本地持久化存储。它自动发现节点上的裸盘，组建 LVM VolumeGroup，并通过 CSI 接口为 Pod 动态分配 LogicalVolume 作为 PersistentVolume，同时支持存储卷的扩容、快照和拓扑感知调度。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **磁盘规划**: 将系统盘和数据盘分开，Carina 只管理数据盘
- **SSD/HDD 分池**: 为 SSD 和 HDD 创建不同的 VolumeGroup 和 StorageClass
- **IO 限速**: 为共享磁盘的工作负载设置 IO 限速，避免互相影响
- **扩容预留**: 初始分配适量空间，利用在线扩容按需增长
- **监控**: 监控各节点 VolumeGroup 的剩余空间，及时扩容或添加磁盘

## 架构定位

在 CNCF 生态中，carina 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[entities/kube-scheduler.md|kube-scheduler]]
- [[entities/csi-drivers.md|csi-drivers]]

## Related

- [[hexa]] — Hexa
- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — [[Podman Desktop|Podman Desktop]]
- [[openyurt]] — OpenYurt
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- carina
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
