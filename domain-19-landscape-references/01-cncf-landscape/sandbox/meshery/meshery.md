---
title: Meshery (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- meshery
- istio
- cilium
- crd
- operator
- kserve
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Meshery 是什么
- 如何 Meshery
trigger_keywords:
- Meshery
prerequisites:
- kubectl-basics
- service-mesh-basics
- cilium-basics
created: "2026-05-23"
---

# Meshery

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, JavaScript

## 概述

Meshery 是云原生管理平面，提供服务网格和云原生基础设施的生命周期管理。它支持多种服务网格 (Istio, Linkerd, Consul, Kuma, NSM 等) 的安装、配置、性能测试和运维管理，并提供统一的 Web 界面和 CLI。Meshery 还定义了 MeshModel 标准，用于描述云原生基础设施。

## 核心能力

- **多网格支持**: 管理 10+ 种服务网格
- **生命周期管理**: 安装、升级、卸载服务网格
- **性能测试**: 内置负载测试和性能比较
- **配置管理**: 统一界面管理网格配置
- **MeshModel**: 云原生基础设施建模标准
- **设计模式**: 预定义的云原生部署模式

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进评估**: 使用 Meshery 对比测试不同网格
- **性能基线**: 在部署网格前建立性能基线
- **模式复用**: 使用设计模式标准化部署
- **多集群视图**: 统一管理多集群网格部署
- **持续测试**: 定期运行性能测试检测退化
- **社区模式**: 贡献和复用社区设计模式

## 架构定位

在 CNCF 生态中，meshery 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]
- [[cilium]]
- [[deployment]]
- [[concepts/service-mesh-architecture|service-mesh-architecture]]

## Related

- [[kserve]] — KServe
- [[istio]] — Istio
- [[kuma]] — Kuma
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- meshery
- [[entities/cncf-networking|[[CNCF 网络与服务网格项目全景|CNCF 网络与服务网格项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
