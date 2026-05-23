---
title: Cedar (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- cedar
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
- Cedar 是什么
- 如何 Cedar
trigger_keywords:
- Cedar
prerequisites:
- kubectl-basics
- gitops-basics
created: "2026-05-23"
---

# Cedar

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Rust

## 概述

Cedar 是一个由 AWS 开发的开源策略语言和评估引擎，用于定义和执行细粒度的访问控制策略。它专为应用程序的授权决策设计，提供人类可读的策略语法、形式化验证工具和高性能的策略评估引擎。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Schema 定义**: 先定义 Entity Schema，确保策略类型安全
- **形式化验证**: 使用 Cedar 分析工具验证策略正确性和完整性
- **forbid 优先**: 先定义禁止策略再定义允许策略，确保安全默认
- **策略模板**: 使用模板减少重复策略定义
- **外部化策略**: 将策略从应用代码中分离，独立管理和部署

## 架构定位

在 CNCF 生态中，cedar 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/argocd.md|[[ArgoCD|argocd]]]]

## Related

- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubescape]] — Kubescape
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cedar
- [[entities/cncf-security|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
