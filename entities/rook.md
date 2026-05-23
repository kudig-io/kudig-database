---
title: Rook (entities)
description: '## 概述'
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
created: "2026-05-23"
---

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

- [[entities/virtual-kubelet.md|kubelet]]]] — [[Virtual Kubelet|Virtual Kubelet]]
- [[kudo]] — KUDO
- [[02-containerd-v2-features]] — containerd 2.0 新特性
- [[karmada]] — Karmada
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- rook
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- RELEASE-NOTES-0.7
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- RELEASE-NOTES-1.1
- [[domain-19-landscape-references/topic-release-notes/storage/rook/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- [[references/release-notes-storage|发布说明索引 — 存储]] — Cross-reference
- [[concepts/storage-tool-evolution|存储工具演进]] — Cross-reference
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
