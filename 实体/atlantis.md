---
title: Atlantis (entities)
description: '## 概述'
summary: 'Atlantis 是一个 Terraform/OpenTofu Pull Request 自动化工具。它监听 Git 仓库的 PR，自动执行 `terraform plan`，并在 PR 中显示执行计划。团队成员可以通过 PR 评论来审查和批准变更，然后通过评论命令执行 `terraform apply`，实现基础设施即代码的协作式工作流。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- atlantis
- prometheus
- grafana
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Atlantis 是什么
- 如何 Atlantis
trigger_keywords:
- Atlantis
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Atlantis

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Atlantis 是一个 Terraform/OpenTofu Pull Request 自动化工具。它监听 Git 仓库的 PR，自动执行 `terraform plan`，并在 PR 中显示执行计划。团队成员可以通过 PR 评论来审查和批准变更，然后通过评论命令执行 `terraform apply`，实现基础设施即代码的协作式工作流。

## 核心能力

- **PR 自动化**: PR 创建时自动执行 terraform plan
- **评论驱动**: 通过 PR 评论控制工作流
- **多 VCS 支持**: GitHub、GitLab、Bitbucket、Azure DevOps
- **工作区隔离**: 支持多工作区并行操作
- **锁定机制**: 防止并发修改同一状态
- **审批流程**: 可配置的 apply 前审批要求

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **分支策略**: 只允许从 main/master 分支 apply
- **审批要求**: 生产环境要求 PR 审批
- **锁定管理**: 定期清理过期的锁
- **Secret 管理**: 使用 Vault 或 AWS [[Secrets|Secrets]] Manager
- **状态后端**: 使用远程状态后端 (S3, GCS)
- **高可用**: 使用持久存储保存 Atlantis 数据

## 架构定位

在 CNCF 生态中，atlantis 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/vault.md|vault]]
- [[deployment]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[dragonfly]] — Dragonfly
- [[aeraki-mesh]] — Aeraki Mesh
- [[opentofu]] — OpenTofu
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- atlantis
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
