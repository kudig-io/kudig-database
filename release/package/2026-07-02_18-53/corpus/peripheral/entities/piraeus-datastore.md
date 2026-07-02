---
title: Piraeus Datastore (entities)
description: '## 概述'
summary: 'Piraeus Datastore 是基于 LINSTOR 和 DRBD 技术的 Kubernetes 高可用存储解决方案。它提供高性能的块存储，支持同步复制、快照、加密和灾难恢复。Piraeus 将成熟的 Linux 存储技术（DRBD 同步复制已有 20+ 年历史）与 Kubernetes 原生体验结合，为有状态应用提供企业级存储。'
category: entities
tags:
- k8s
- cncf
- storage
- piraeus-datastore
- ingress
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Piraeus Datastore 是什么
- 如何 Piraeus Datastore
trigger_keywords:
- Piraeus
- Datastore
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Piraeus Datastore

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go, Java

## 概述

Piraeus Datastore 是基于 LINSTOR 和 DRBD 技术的 Kubernetes 高可用存储解决方案。它提供高性能的块存储，支持同步复制、快照、加密和灾难恢复。Piraeus 将成熟的 Linux 存储技术（DRBD 同步复制已有 20+ 年历史）与 Kubernetes 原生体验结合，为有状态应用提供企业级存储。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **专用存储节点**: 为存储工作负载配置专用节点，避免与计算混部
- **SSD/NVMe**: 使用 SSD 或 NVMe 获得最佳性能
- **副本规划**: 生产环境至少 2 副本，跨可用区部署 3 副本
- **监控 DRBD**: 监控 DRBD 同步状态，及时发现脑裂或同步延迟
- **定期快照**: 配置定期快照策略，保护关键数据

## 架构定位

在 CNCF 生态中，piraeus-datastore 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[entities/csi-drivers.md|csi-drivers]]

## Related

- [[spin]] — Spin
- [[backstage]] — Backstage
- [[entities/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- piraeus-datastore
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
