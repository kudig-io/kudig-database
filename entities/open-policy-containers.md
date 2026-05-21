---
title: Open Policy Containers (OPCR)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- policy
- open-policy-containers
- opa
- crd
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Policy Containers (OPCR) 是什么
- 如何 Open Policy Containers (OPCR)
trigger_keywords:
- Open
- Policy
- Containers
- OPCR
prerequisites:
- kubectl-basics
- policy-basics
---

# Open Policy Containers (OPCR)

> **CNCF 状态**: Sandbox | **类别**: Policy | **主要语言**: Go

## 概述

Open Policy Containers (OPCR) 是一个将 OPA (Open Policy Agent) 策略打包为 OCI 兼容镜像并分发的标准和工具集。它定义了 Policy as Code 的打包格式，使策略可以像容器镜像一样存储在任意 OCI Registry 中，并支持签名、版本化和分发。OPCR 让安全策略的管理和部署与云原生工作流无缝集成。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **语义版本**: 使用语义化版本号管理策略版本
- **签名验证**: 生产环境始终验证策略签名，防止策略被篡改
- **测试先行**: 每次策略变更都运行完整的测试套件
- **分层策略**: 将通用策略和业务策略分开打包，便于复用
- **审计日志**: 记录策略版本变更和部署历史

## 架构定位

在 CNCF 生态中，open-policy-containers 属于 **Policy** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[artifact-hub]] — Artifact Hub
- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/open-policy-containers/open-policy-containers.md|open-policy-containers]]
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
