---
title: KusionStack (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- platform
- kusionstack
- containerd
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KusionStack 是什么
- 如何 KusionStack
trigger_keywords:
- KusionStack
prerequisites:
- kubectl-basics
- iac-basics
- observability-basics
created: "2026-05-23"
---

# KusionStack

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go, KCL

## 概述

KusionStack 是一个云原生可编程技术栈，提供以应用为中心的配置管理和交付能力。它使用 KCL (Kusion Configuration Language) 作为配置语言，结合 Kusion 引擎实现从应用配置到多云/多环境的一致性交付。KusionStack 支持 Kubernetes、Terraform 等多种 IaC 后端，让平台团队可以为开发者提供简化的自助式应用交付体验。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **项目结构**: 按环境组织 stack，共享配置放在项目级别
- **模块复用**: 将通用配置封装为 Konfig 模块，团队内共享
- **约束前置**: 使用 KCL schema 约束在编写阶段捕获配置错误
- **Preview 必做**: 在 apply 前始终执行 preview 确认变更影响
- **CI/CD 集成**: 将 kusion preview/apply 集成到 GitOps 流程

## 架构定位

在 CNCF 生态中，kusionstack 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[crossplane]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/storage-model.md|storage-model]]

## Related

- [[06-containerd-observability]] — containerd 可观测性
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kusionstack
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
