---
title: OpenTofu (entities)
description: '## 概述'
summary: 'OpenTofu 是 Terraform 的开源分支，在 Terraform 转向 BSL 许可后由社区创建。它是一个基础设施即代码 (IaC) 工具，允许使用声明式配置语言定义和管理云资源。'
category: entities
tags:
- k8s
- cncf
- config
- opentofu
- containerd
- harbor
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenTofu 是什么
- 如何 OpenTofu
trigger_keywords:
- OpenTofu
prerequisites:
- kubectl-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenTofu

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Go

## 概述

OpenTofu 是 Terraform 的开源分支，在 Terraform 转向 BSL 许可后由社区创建。它是一个基础设施即代码 (IaC) 工具，允许使用声明式配置语言定义和管理云资源。

## 核心能力

- **完全兼容**: 与 Terraform 1.5.x 配置兼容
- **开源许可**: MPL-2.0 开源许可证
- **状态管理**: 支持本地和远程状态后端
- **模块系统**: 可复用的基础设施模块
- **Provider 生态**: 兼容所有 Terraform Providers
- **社区驱动**: Linux Foundation 托管，社区治理

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **状态锁定**: 使用 DynamoDB 或等效机制防止并发修改
- **版本控制**: 将配置文件纳入 Git 管理
- **模块化**: 使用模块组织可复用的基础设施
- **变量管理**: 使用 tfvars 文件管理环境差异
- **敏感数据**: 使用 sensitive 标记或外部密钥管理

## 架构定位

在 CNCF 生态中，opentofu 属于 **Config** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[kubeedge]] — KubeEdge
- [[telepresence]] — Telepresence
- [[08-containerd-multi-tenant]] — containerd 多租户
- [[harbor]] — Harbor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opentofu
- [[entities/cdk8s.md|cdk8s (Cloud Development Kit for Kubernetes)]]
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
