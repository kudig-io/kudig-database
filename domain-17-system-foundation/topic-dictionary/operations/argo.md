---
title: Argo
description: 'Argo 是 CNCF 毕业项目集合，包含 Argo CD（GitOps 持续部署）、Argo Workflows（容器原生工作流引擎）、Argo Rollou...'
category: dictionary
tags:
- k8s
- glossary
- argo
- gitops
- cicd
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Argo 是什么
- Argo 详解
trigger_keywords:
- Argo
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Argo

> **英文名**: Argo

## 概述

Argo 是 CNCF 毕业项目集合，包含 Argo CD（GitOps 持续部署）、Argo Workflows（容器原生工作流引擎）、Argo Rollouts（渐进式发布）和 Argo Events（事件驱动）。Argo CD 是 Kubernetes 生态中最主流的 GitOps 工具。

## 核心概念/原理

### Argo 项目家族

| 项目 | 功能 | 成熟度 |
|------|------|--------|
| **Argo CD** | GitOps 持续部署 | Graduated |
| **Argo Workflows** | 容器原生 DAG 工作流 | Graduated |
| **Argo Rollouts** | 金丝雀/蓝绿发布 | Incubating |
| **Argo Events** | 事件驱动自动化 | Incubating |

### Argo CD 核心概念

- **Application**：声明式的 GitOps 应用定义。
- **Sync Policy**：自动或手动将 Git 变更同步到集群。
- **Health Check**：自定义资源健康状态判断。
- **Hook**：Pre/Post Sync 操作（如数据库迁移）。

## 关键机制或特性

- **GitOps 模型**：Git 仓库作为唯一的真实来源（Single Source of Truth）。
- **Pull 模式**：Argo CD 主动从 Git 拉取变更，而非 CI push。
- **多集群管理**：ApplicationSet 批量管理多集群部署。
- **渐进式发布**：Argo Rollouts 支持金丝雀和蓝绿部署策略。
- **SSO/RBAC**：集成 OIDC/LDAP 和应用级 RBAC。

## 使用场景与最佳实践

- 使用 Argo CD 管理所有 K8s 资源的 GitOps 部署。
- 配置 auto-sync + self-heal 实现全自动运维。
- 使用 ApplicationSet 管理多环境/多集群部署。
- 配合 Argo Rollouts 实现金丝雀发布。
- 启用 Argo CD 的 RBAC 和 SSO 控制访问权限。

## 参考链接

- [Argo CD Official](https://argo-cd.readthedocs.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/operations/rolling-update.md|Rolling Update]]
- [[domain-17-system-foundation/topic-dictionary/operations/rollback.md|Rollback]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
