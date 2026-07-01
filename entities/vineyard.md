---
title: Vineyard
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- data
- vineyard
- crd
- operator
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
- Vineyard 是什么
- 如何 Vineyard
trigger_keywords:
- Vineyard
prerequisites:
- kubectl-basics
---



# Vineyard

> **CNCF 状态**: Sandbox | **类别**: Data | **主要语言**: C++, Python

## 概述

Vineyard 是一个内存中的不可变数据管理器，为大数据和 AI/ML 工作流提供零拷贝数据共享。它通过共享内存机制在同一节点上的不同计算引擎（如 Spark、PyTorch、Dask、GraphScope）之间实现高效数据传递，避免了传统方式中序列化/反序列化和磁盘 IO 的开销，可将数据流水线的端到端性能提升数倍。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **内存规划**: 根据数据集大小合理配置 Vineyard 的共享内存池大小
- **数据生命周期**: 及时释放不再使用的对象，避免内存泄漏
- **亲和性调度**: 在 K8s 中将有数据依赖的 Pod 调度到同一节点
- **分布式模式**: 大数据集使用分布式 Vineyard，数据分片存储在多个节点

## 架构定位

在 CNCF 生态中，vineyard 属于 **Data** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]
- [[pod-lifecycle]]

## Related

- [[hami]] — HAMI
- [[open-policy-containers]] — [[entities/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[werf]] — werf
- [[dalec]] — Dalec
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- vineyard
- storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
