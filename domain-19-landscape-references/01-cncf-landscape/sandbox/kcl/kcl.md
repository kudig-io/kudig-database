---
title: KCL (Kusion Configuration Language)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- config
- kcl
- argocd
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KCL (Kusion Configuration Language) 是什么
- 如何 KCL (Kusion Configuration Language)
trigger_keywords:
- KCL
- Kusion
- Configuration
- Language
prerequisites:
- kubectl-basics
- gitops-basics
created: "2026-05-23"
---

# KCL (Kusion Configuration Language)

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Rust, Go

## 概述

KCL (Kusion Configuration Language) 是一个开源的基于约束的记录与函数式配置语言，专为云原生配置和策略管理设计。它提供类型系统、schema 约束、配置合并和覆盖等能力，帮助团队以编程方式管理复杂的 Kubernetes 和云基础设施配置，减少配置错误。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Schema 优先**: 先定义 Schema 和约束规则，再编写具体配置
- **模块化**: 按功能拆分 KCL 文件，使用包管理组织代码
- **环境覆盖**: 使用配置合并实现环境差异化（dev/staging/prod）
- **策略验证**: 编写安全和合规策略，在 CI 阶段拦截违规配置
- **版本管理**: 使用 OCI Registry 发布和管理 KCL 包版本
- **IDE 工具**: 使用 VS Code KCL 插件获得类型提示和错误检查

## 架构定位

在 CNCF 生态中，kcl 属于 **Config** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/argocd|[[ArgoCD|argocd]]]]
- [[deployment]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[concepts/gitops-principles|gitops-principles]]
- [[concepts/security-defense-depth|security-defense-depth]]

## Related

- [[parsec]] — Parsec
- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[tuf]] — TUF
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kcl
- [[entities/kpt|kpt]]
- [[entities/cdk8s|cdk8s (Cloud Development Kit for Kubernetes)]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
