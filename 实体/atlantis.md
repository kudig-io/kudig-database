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
last_updated: 2026-07
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

Atlantis 是由 Hootsuite 开源（现由社区维护）的 Terraform/OpenTofu Pull Request 自动化工具。它监听 Git 仓库的 PR，自动执行 `terraform plan`，并在 PR 中显示执行计划。团队成员可以通过 PR 评论审查和批准变更，然后通过评论命令执行 `terraform apply`，实现基础设施即代码（IaC）的协作式工作流。Atlantis 是 Terraform 社区最受欢迎的 PR 自动化方案。

## 核心特性

- **PR 自动化**: PR 创建/更新时自动执行 terraform plan
- **评论驱动**: 通过 `atlantis plan`、`atlantis apply` 等 PR 评论控制工作流
- **多 VCS 支持**: GitHub、GitLab、Bitbucket、Azure DevOps
- **多工作区**: 支持多 Terraform workspace 并行操作
- **项目锁定**: 自动锁定正在操作的项目，防止并发修改
- **自定义工作流**: 支持自定义 plan/apply 脚本和步骤

## 架构

Atlantis 是一个用 Go 编写的 Web 服务。核心流程：Atlantis 配置 Webhook 接收 Git 仓库的 PR 事件，解析 PR 变更中涉及的 Terraform 目录，在本地检出代码执行 `terraform init && terraform plan -out planfile`，将 plan 输出作为 PR 评论发布。当用户评论 `atlantis apply` 时，Atlantis 执行 `terraform apply planfile` 并将结果评论到 PR。Atlantis 在本地维护每个项目的锁状态，防止并发操作。所有 Terraform 操作在 Atlantis 容器内执行。

## Kubernetes 集成

Atlantis 以 Deployment 部署到 Kubernetes。通过 Service 暴露 Webhook 接收端点。使用 PVC 或持久卷存储 Terraform 状态缓存和锁数据。通过 Kubernetes Secret 管理 VCS Token 和云凭证。支持 Helm Chart 部署。与 ArgoCD/FluxCD 配合时，Atlantis 负责 plan/apply，GitOps 工具负责将 Terraform state 变更同步到集群。Ingress 暴露 Webhook 端点。

## 生产使用场景

1. **基础设施 PR 审查**: 团队成员通过 PR 协作审查基础设施变更
2. **自动化 plan/apply**: 消除手动执行 Terraform 命令的繁琐流程
3. **多环境管理**: 为 dev/staging/prod 配置不同的工作区和审批规则
4. **合规审计**: PR 评论中保留完整的 plan 输出和 apply 记录

## 安装

```bash
# Helm 安装
helm repo add atlantis https://runatlantis.github.io/helm-charts
helm install atlantis atlantis/atlantis \
  --set github.user=<bot-username> \
  --set github.token=<github-token> \
  --set github.secret=<webhook-secret> \
  --set ingress.enabled=true \
  --set ingress.host=atlantis.example.com
# atlantis.yaml 配置
version: 3
projects:
- dir: infrastructure/
  workflow: production
  apply_requirements: [approved]
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Atlantis** | 开源、多 VCS | 需自托管 |
| Terraform Cloud | SaaS、远程状态 | 商业产品 |
| Digger | 开源、支持 GitHub Actions | 社区较小 |
| GitHub Actions | 原生 CI/CD | 需自行实现工作流 |

## 架构定位

在 DevOps 生态中，Atlantis 属于 **IaC Automation** 类别，是 Terraform PR 协作工作流的标准方案。它填补了 GitOps 工具链中 IaC 自动化的空白。

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
