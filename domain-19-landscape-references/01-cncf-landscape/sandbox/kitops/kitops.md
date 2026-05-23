---
title: KitOps (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- image
- kitops
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KitOps 是什么
- 如何 KitOps
trigger_keywords:
- KitOps
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# KitOps

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

KitOps 是一个 MLOps/AI 工件打包和版本管理工具，使用 OCI 标准将 AI/ML 项目的所有组件（模型权重、数据集、代码、配置）打包为称为 ModelKit 的 OCI Artifact。它允许数据科学家和 ML 工程师像管理容器镜像一样管理 AI 模型全生命周期的工件，并通过标准容器注册中心进行分发。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **版本策略**: 使用语义版本标签管理 ModelKit，保持可追溯性
- **分层打包**: 将大文件（模型权重）放在独立层，利用 OCI 层缓存加速拉取
- **元数据完善**: 在 Kitfile 中详细记录模型参数、训练配置和评估指标
- **CI/CD 集成**: 训练完成后自动打包推送 ModelKit
- **安全扫描**: 对 ModelKit 中的代码部分进行安全扫描

## 架构定位

在 CNCF 生态中，kitops 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/secrets-management|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns|ci-cd-pipeline-patterns]]

## Related

- [[slimfaas]] — SlimFaas
- [[tuf]] — TUF
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kube-vip]] — kube-vip
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kitops
- [[entities/slimtoolkit|[[SlimToolkit|SlimToolkit]]]]
- [[entities/modelpack|ModelPack]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
