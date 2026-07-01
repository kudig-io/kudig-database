---
title: Runme (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- platform
- runme-notebooks
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
- Runme 是什么
- 如何 Runme
trigger_keywords:
- Runme
prerequisites:
- kubectl-basics
---



# Runme

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go, TypeScript

## 概述

Runme 是一个交互式 Markdown 运行时，可以将 Markdown 文档中的代码块转化为可执行的交互式笔记本。它让开发者可以直接在 VS Code 中运行 README、runbook 和文档中的命令，并保存执行结果。Runme 特别适合 DevOps、SRE 运维手册和开发文档的交互式执行。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **文档即代码**: 将 runbook 和文档作为代码纳入版本控制
- **环境隔离**: 使用 Runme 的环境变量功能隔离不同环境配置
- **分段执行**: 将长流程拆分为多个单元格，便于调试和复用
- **结果保存**: 保存执行输出，便于问题排查和审计
- **协作分享**: 使用 Runme Cloud 分享带结果的 Notebook

## 架构定位

在 CNCF 生态中，runme-notebooks 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kpt]] — kpt
- [[logging-operator]] — Loggingng Operator|Logging Operator]]
- [[kubeclipper]] — KubeClipper
- [[README]] — FTA 故障树清单索引
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- runme-notebooks
- [[entities/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
