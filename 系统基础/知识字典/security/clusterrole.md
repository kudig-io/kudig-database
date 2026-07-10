---
title: 集群角色
description: ClusterRole 是 Kubernetes RBAC 中集群级别的权限定义资源。与 Role 不同，ClusterRole 不受命名空间限制，可以授予集群...
summary: ClusterRole 是 Kubernetes RBAC 中集群级别的权限定义资源。与 Role 不同，ClusterRole 不受命名空间限制，可以授予集群...
category: dictionary
tags:
- k8s
- glossary
- clusterrole
- rbac
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 集群角色 是什么
- ClusterRole 详解
trigger_keywords:
- 集群角色
- ClusterRole
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群角色

> **英文名**: ClusterRole

## 概述

ClusterRole 是 Kubernetes RBAC 中集群级别的权限定义资源。与 Role 不同，ClusterRole 不受命名空间限制，可以授予集群范围和跨命名空间的权限。

## 核心概念/原理

### 核心概念

- **集群范围权限**：可以访问集群级别资源（Node、PV、ClusterRole 等）。
- **跨命名空间权限**：通过 ClusterRoleBinding 可以在所有命名空间生效。
- **命名空间范围使用**：ClusterRole 也可以通过 RoleBinding 限制在特定命名空间内使用。
- **聚合 ClusterRole**：使用 `aggregationRule` 自动合并多个 ClusterRole 的规则。

### 内置 ClusterRole

Kubernetes 预定义了一些常用的 ClusterRole：
- `cluster-admin`：完全管理员权限。
- `admin`：命名空间管理员。
- `edit`：命名空间内读写。
- `view`：命名空间内只读。

## 关键机制或特性

- 聚合 ClusterRole 会自动包含匹配标签的其他 ClusterRole 的规则。
- `admin`、`edit`、`view` 是推荐的预定义角色。
- ClusterRole 可以授予对非资源 URL（如 `/healthz`）的访问权限。

## 使用场景与最佳实践

- 避免过度使用 `cluster-admin`，优先使用最小权限的自定义 ClusterRole。
- 使用预定义的 `view`/`edit`/`admin` 角色简化权限管理。
- 定期使用 `kubectl auth can-i --list` 审计权限。

## 参考链接

- [ClusterRole - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[系统基础/知识字典/security/rbac.md|Rbac]]
- [[系统基础/知识字典/security/rolebinding.md|Rolebinding]]
- [[系统基础/知识字典/security/clusterrolebinding.md|Clusterrolebinding]]
- [[系统基础/知识字典/security/service-account.md|Service Account]]
- [[系统基础/知识字典/security/service-account-token.md|Service Account Token]]


<!-- risk-assessed -->
