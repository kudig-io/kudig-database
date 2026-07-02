---
title: HwameiStor (entities)
description: '## 概述'
summary: 'HwameiStor 是一个 Kubernetes 原生的高可用本地存储系统，能够将节点上的本地磁盘（HDD、SSD、NVMe）统一管理并提供分布式的本地存储服务。它通过 CSI 接口为有状态应用提供高性能的本地持久卷，并支持卷的高可用副本、数据迁移和自动化运维。HwameiStor 特别适合对 IOPS 和延迟敏感的工作负载，'
category: entities
tags:
- k8s
- cncf
- storage
- hwameistor
- crd
- operator
- serverless
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HwameiStor 是什么
- 如何 HwameiStor
trigger_keywords:
- HwameiStor
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HwameiStor

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

HwameiStor 是一个 Kubernetes 原生的高可用本地存储系统，能够将节点上的本地磁盘（HDD、SSD、NVMe）统一管理并提供分布式的本地存储服务。它通过 CSI 接口为有状态应用提供高性能的本地持久卷，并支持卷的高可用副本、数据迁移和自动化运维。HwameiStor 特别适合对 IOPS 和延迟敏感的工作负载，如数据库和 AI/ML 训练任务。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **磁盘分类**: 将 SSD 和 HDD 分配到不同存储池，按性能需求选择 StorageClass
- **副本拓扑**: 配置副本拓扑约束确保副本分布在不同故障域
- **监控告警**: 监控 LocalDisk 和 LocalVolume 的 conditions，及时发现问题
- **定期迁移**: 在节点维护前提前进行卷迁移，减少业务影响
- **资源预留**: 为每个节点预留足够的磁盘空间用于数据重建

## 架构定位

在 CNCF 生态中，hwameistor 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[entities/csi-drivers.md|csi-drivers]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[strimzi]] — Strimzi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hwameistor
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
