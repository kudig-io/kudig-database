---
title: Atlantis Terraform 自动化
description: Atlantis 是开源的 Terraform Pull Request 自动化工具，在 PR 中自动执行 terraform plan/apply，为基础设施...
summary: Atlantis 是开源的 Terraform Pull Request 自动化工具，在 PR 中自动执行 terraform plan/apply，为基础设施...
category: dictionary
tags:
- k8s
- glossary
- tooling
- terraform
- ci-cd
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Atlantis Terraform 自动化 是什么
- Atlantis 详解
trigger_keywords:
- Atlantis Terraform 自动化
- Atlantis
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Atlantis Terraform 自动化（Atlantis）

## 概述

Atlantis 是开源的 Terraform Pull Request 自动化工具，在 PR 中自动执行 terraform plan/apply，为基础设施变更提供代码审查和自动化部署工作流。

## 核心概念/原理

- **PR 驱动**：在 PR 中自动运行 terraform plan
- **审查流程**：基础设施变更的代码审查和批准
- **多仓库**：支持多个 Terraform 项目
- **社区成熟**：广泛使用的 Terraform CI/CD 方案

## 关键机制或特性

- Webhook 监听 PR 事件
- 自动检测变更的 Terraform 目录
- `atlantis plan` 在 PR 评论中展示计划
- `atlantis apply` 在 PR 批准后执行
- 多 workspace/目录管理
- 支持 Terragrunt/OpenTofu
- 自定义工作流（pre/post hooks）

## 使用场景与最佳实践

- Terraform 变更的 PR 审查流程
- 基础设施变更的自动化部署
- 多团队协作的 Terraform 管理
- GitOps 式的基础设施管理
- 合规要求下的变更审计

## 参考链接

- https://www.runatlantis.io/
- https://github.com/runatlantis/atlantis

## Related

- [[系统基础/知识字典/tooling/opentofu.md|OpenTofu]]
- [[系统基础/知识字典/platform-engineering/crossplane.md|Crossplane]]
- [[系统基础/知识字典/operations/argo.md|Argo]]


<!-- risk-assessed -->
