---
title: GitOps
description: GitOps 是一种以 Git 仓库作为基础设施和应用配置的唯一真实来源（Single Source of Truth）的运维方法论。通过声明式配置和自动化拉取...
summary: GitOps 是一种以 Git 仓库作为基础设施和应用配置的唯一真实来源（Single Source of Truth）的运维方法论。通过声明式配置和自动化拉取...
category: dictionary
tags:
- k8s
- glossary
- gitops
- cicd
- methodology
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GitOps 是什么
- GitOps 详解
trigger_keywords:
- GitOps
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# GitOps

> **英文名**: GitOps

## 概述

GitOps 是一种以 Git 仓库作为基础设施和应用配置的唯一真实来源（Single Source of Truth）的运维方法论。通过声明式配置和自动化拉取（Pull）模式，实现基础设施即代码（IaC）的持续交付。

## 核心概念/原理

### 核心原则

| 原则 | 说明 |
|------|------|
| 声明式 | 所有配置以期望状态描述 |
| 版本化 | Git 作为配置的唯一来源 |
| 自动拉取 | 控制器主动从 Git 拉取变更 |
| 持续调谐 | 实际状态持续向期望状态收敛 |

### Push vs Pull 模式

- **Push**：CI 流水线直接 `kubectl apply`（传统方式）
- **Pull**：集群内控制器从 Git 拉取并同步（GitOps 方式）

## 关键机制或特性

- **Argo CD**：最流行的 GitOps 控制器，支持多集群管理。
- **Flux**：CNCF 毕业项目，原生多租户支持。
- **Kustomize/Helm**：GitOps 中常用的配置渲染工具。
- **密封密钥（Sealed Secrets）**：在 Git 中安全存储加密的 Secret。
- 支持渐进式发布（配合 Argo Rollouts/Flagger）。

## 使用场景与最佳实践

- 所有 K8s 资源定义存放在 Git 仓库中，通过 PR 管理变更。
- 使用 Argo CD 或 Flux 实现自动同步。
- 敏感信息使用 Sealed Secrets 或 External Secrets Operator。
- 环境分离策略：按目录或按分支管理多环境配置。
- 配置漂移检测：GitOps 控制器自动检测并修复漂移。

## 参考链接

- [OpenGitOps](https://opengitops.dev/)

## Related

- [[domain-17-system-foundation/知识字典/operations/argo.md|Argo]]
- [[domain-17-system-foundation/知识字典/operations/flux.md|Flux]]
- [[domain-17-system-foundation/知识字典/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/知识字典/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/知识字典/workloads/deployment.md|Deployment]]


<!-- risk-assessed -->
