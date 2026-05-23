---
title: gRPC (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- grpc
- etcd
- istio
- crd
- operator
- argocd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gRPC 是什么
- 如何 gRPC
trigger_keywords:
- gRPC
prerequisites:
- kubectl-basics
- service-mesh-basics
- gitops-basics
- etcd-basics
created: "2026-05-23"
---

# gRPC

> **CNCF 状态**: Incubating | **类别**: Networking | **主要语言**: C++, Go, Java, Python 等

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，grpc 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[istio]]

## Related

- [[46-terway-performance-tuning]] — Terway 性能调优
- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- grpc
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[references/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
