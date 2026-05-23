---
title: Clusterpedia [entities]
description: 'summary: "Clusterpedia 是一个多集群资源的统一搜索和查询引擎，类似于 Kubernetes 资源的 "百科全书"。它将多个集群的资源同步到统一的存储中，提供与 kubectl 兼容的 API 进行跨集群的资源搜索、过滤和分页查询。"'
category: general
tags:
- k8s
- postgresql
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Clusterpedia 是什么
- 如何 Clusterpedia
trigger_keywords:
- Clusterpedia
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

---
title: "Clusterpedia"
category: entities
summary: "Clusterpedia 是一个多集群资源的统一搜索和查询引擎，类似于 Kubernetes 资源的 "百科全书"。它将多个集群的资源同步到统一的存储中，提供与 kubectl 兼容的 API 进行跨集群的资源搜索、过滤和分页查询。"
tags: k8s, cncf, orchestration, clusterpedia]
sources: ["docs/domain-19-landscape-references/sandbox/clusterpedia/clusterpedia.md", "domain-19-landscape-references/sandbox/clusterpedia/clusterpedia.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# Clusterpedia

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Clusterpedia 是一个多集群资源的统一搜索和查询引擎，类似于 Kubernetes 资源的 "百科全书"。它将多个集群的资源同步到统一的存储中，提供与 kubectl 兼容的 API 进行跨集群的资源搜索、过滤和分页查询。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **资源选择**: 只同步需要查询的资源类型，减少存储和同步开销
- **存储后端**: 大规模场景使用 PostgreSQL，小规模使用内置存储
- **标签查询**: 利用 search label 实现复杂的跨集群查询
- **增量同步**: Clusterpedia 使用增量同步，对源集群影响极小
- **权限控制**: 配置 RBAC 限制用户可查询的集群和资源范围

## 架构定位

在 CNCF 生态中，clusterpedia 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[crd-custom-resources]]
- [[pod-lifecycle]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
