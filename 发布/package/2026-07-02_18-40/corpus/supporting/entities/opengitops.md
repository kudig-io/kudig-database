---
title: OpenGitOps [entities]
description: '## 概述'
summary: 'OpenGitOps 是一个 CNCF Sandbox 项目，定义了 GitOps 的标准原则和最佳实践。它并非一个软件工具，而是一组社区驱动的 GitOps 规范和标准，为 GitOps 实践提供厂商中立的定义和指南。'
category: entities
tags:
- k8s
- cncf
- platform
- opengitops
- argocd
- flux
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenGitOps 是什么
- 如何 OpenGitOps
trigger_keywords:
- OpenGitOps
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenGitOps

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

OpenGitOps 是一个 CNCF Sandbox 项目，定义了 GitOps 的标准原则和最佳实践。它并非一个软件工具，而是一组社区驱动的 GitOps 规范和标准，为 GitOps 实践提供厂商中立的定义和指南。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Git 作为唯一来源**: 所有配置变更通过 Git PR/MR 流程管理
- **不可变部署**: 使用镜像 digest 而非 mutable tag (如 latest)
- **自动协调**: 部署工具应持续监控并纠正状态漂移
- **分离仓库**: 应用代码和部署配置使用独立的 Git 仓库
- **审计追踪**: 利用 Git 历史提供完整的变更审计日志

## 架构定位

在 CNCF 生态中，opengitops 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[flux]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/declarative-api.md|declarative-api]]

## Related

- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[krkn]] — Krkn
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opengitops
- [[concepts/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
