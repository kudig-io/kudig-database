---
title: xRegistry (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- image
- xregistry
- crd
- operator
- kubeflow
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- xRegistry 是什么
- 如何 xRegistry
trigger_keywords:
- xRegistry
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# xRegistry

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

xRegistry 是一个通用的元数据注册中心规范，用于管理和发现事件驱动架构中的各类资源。它定义了一种标准化的 API 来注册、存储和查询消息定义、模式（Schema）、端点等元数据，支持 CloudEvents、AsyncAPI、OpenAPI 等多种规范，是构建可互操作事件驱动系统的基础设施。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- com.example.orders.created
- com.example.orders.updated
- com.example.orders.cancelled
- com.example.inventory.reserved
- com.example.payments.completed
- order-created-data-v1

## 架构定位

在 CNCF 生态中，xregistry 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubeelasti]] — [[entities/kubeelasti.md|KubeElastic]]
- [[cloudevents]] — CloudEvents
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- xregistry
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
