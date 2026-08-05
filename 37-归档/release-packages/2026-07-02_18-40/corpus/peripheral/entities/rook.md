---
title: Rook (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- storage
- rook
- kubelet
- prometheus
- grafana
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Rook 是什么
- 如何 Rook
trigger_keywords:
- Rook
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Rook

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，rook 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]]
- [[concepts/storage-model.md|storage-model]]

## Related

- [[entities/virtual-kubelet.md|kubelet]]]] — Virtual Kubelet
- [[kudo]] — KUDO
- [[32-发布/package/2026-07-02_18-40/corpus/core/entities/01-containerd-v2-features]] — containerd 2.0 新特性
- [[karmada]] — Karmada
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- rook
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- RELEASE-NOTES-0.7
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- RELEASE-NOTES-1.1
- [[domain-19-landscape-references/_archives/release-notes/storage/rook/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- [[entities/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[concepts/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
