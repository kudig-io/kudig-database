---
title: OSCAL Compass (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- oscal-compass
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OSCAL Compass 是什么
- 如何 OSCAL Compass
trigger_keywords:
- OSCAL
- Compass
prerequisites:
- kubectl-basics
- policy-basics
---



# OSCAL Compass

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Python

## 概述

OSCAL Compass 是一套基于 NIST OSCAL (Open Security Controls Assessment Language) 标准的合规自动化工具集。它包括 Trestle (合规即代码框架)、C2P (合规到策略转换) 等组件，帮助组织将安全合规要求转换为可执行的代码和策略，实现从合规框架（如 FedRAMP、SOC 2、ISO 27001）到实际控制实施的自动化...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **合规即代码**: 将所有合规文档纳入 Git 版本控制
- **自动化验证**: 在 CI/CD 中自动验证 OSCAL 文档和生成的策略
- **持续监控**: 定期运行评估，持续收集合规证据
- **模块化设计**: 将通用控制封装为可复用的组件定义
- **审计追踪**: 使用 OSCAL Assessment Results 记录所有合规评估

## 架构定位

在 CNCF 生态中，oscal-compass 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[kyverno]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- oscal-compass
- [[entities/openfga.md|[[OpenFGA|OpenFGA]]]]
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
