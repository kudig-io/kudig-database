---
title: Radius
description: 'summary: "Radius 是由 Microsoft 开发的云原生应用平台，提供了一种以应用为中心的方式来定义和部署云原生应用及其依赖的基础设施。它引入了 "Application Graph" 的概念，让开发者定义应用需要什么（如数据库、消息队列），而由平台工程师定义如何提供这些资源（Azure
  CosmosDB 还是本地 MongoDB），实现关注点分离。"'
category: general
tags:
- k8s
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Radius 是什么
- 如何 Radius
trigger_keywords:
- Radius
prerequisites:
- kubectl-basics
---

---
title: "Radius"
category: entities
summary: "Radius 是由 Microsoft 开发的云原生应用平台，提供了一种以应用为中心的方式来定义和部署云原生应用及其依赖的基础设施。它引入了 "Application Graph" 的概念，让开发者定义应用需要什么（如数据库、消息队列），而由平台工程师定义如何提供这些资源（Azure CosmosDB 还是本地 MongoDB），实现关注点分离。"
tags: [k8s, cncf, platform, radius]
sources: ["docs/domain-19-landscape-references/sandbox/radius/radius.md", "domain-19-landscape-references/sandbox/radius/radius.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: draft
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# Radius

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

Radius 是由 Microsoft 开发的云原生应用平台，提供了一种以应用为中心的方式来定义和部署云原生应用及其依赖的基础设施。它引入了 "Application Graph" 的概念，让开发者定义应用需要什么（如数据库、消息队列），而由平台工程师定义如何提供这些资源（Azure CosmosDB 还是本地 MongoDB），实现关注点分离。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **关注点分离**: 开发者通过 Portable Resource 声明需求，平台团队维护 Recipe
- **环境分级**: 为 dev/staging/production 创建不同的 Environment 和 Recipe
- **Recipe 标准化**: 将 Recipe 作为 OCI Artifact 管理，确保基础设施配置一致
- **应用图**: 利用 Application Graph 可视化和理解应用的依赖关系
- **渐进采纳**: 从新应用开始使用 Radius，逐步将已有应用迁移

## 架构定位

在 CNCF 生态中，radius 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[crossplane]]
- [[secrets-management]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
