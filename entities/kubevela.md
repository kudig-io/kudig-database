---
title: KubeVela [entities]
description: '## 概述'
summary: 'KubeVela 是现代应用交付平台，实现了开放应用模型（OAM）规范。它为开发者提供以应用为中心的抽象，简化 Kubernetes 上的应用部署、运维和多集群管理。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubevela
- prometheus
- grafana
- helm
- argocd
- flux
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeVela 是什么
- 如何 KubeVela
trigger_keywords:
- KubeVela
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeVela

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeVela 是现代应用交付平台，实现了开放应用模型（OAM）规范。它为开发者提供以应用为中心的抽象，简化 Kubernetes 上的应用部署、运维和多集群管理。

## 核心能力

- **应用抽象**: 以应用为中心，屏蔽底层 Kubernetes 复杂性
- **OAM 模型**: 组件、特征、策略的标准化定义
- **多集群交付**: 统一管理多个 Kubernetes 集群的应用
- **GitOps**: 与 Flux/ArgoCD 集成实现 GitOps 工作流
- **可扩展**: CUE 语言定义自定义组件和特征
- **工作流**: 内置应用交付工作流引擎

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模块化组件**: 使用 Helm/Kustomize 复用现有配置
- **环境隔离**: 通过 Policy 实现多环境差异化配置
- **渐进交付**: 使用 Workflow 实现分阶段发布
- **GitOps**: 将 Application YAML 存储在 Git 仓库
- **可观测性**: 集成 Prometheus/Grafana 监控应用状态

## 架构定位

在 CNCF 生态中，kubevela 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[flux]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[flux]] — Flux
- [[helm]] — Helm
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD

- kubevela
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
