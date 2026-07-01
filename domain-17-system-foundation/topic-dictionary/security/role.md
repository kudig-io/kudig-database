---
title: 角色
description: Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。...
summary: Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。...
category: dictionary
tags:
- k8s
- glossary
- role
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
- 角色 是什么
- Role 详解
trigger_keywords:
- 角色
- Role
- dictionary
prerequisites:
- kubectl-basics
---



# 角色

> **英文名**: Role

## 概述

Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。

## 核心概念/原理

### 核心概念

- **verbs**：允许的操作（`get`, `list`, `watch`, `create`, `update`, `patch`, `delete`）。
- **resources**：可操作的资源类型（`pods`, `services`, `deployments` 等）。
- **resourceNames**：限定特定资源实例名称。
- **apiGroups**：API 组（`""` 表示核心组，`apps` 表示 apps 组等）。

### 示例

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: default
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
```

## 关键机制或特性

- Role 仅在命名空间内生效。
- ClusterRole 在集群范围生效。
- Role 和 ClusterRole 都通过 Binding 关联到用户/组/ServiceAccount。

## 使用场景与最佳实践

- 遵循最小权限原则。
- 优先使用 ClusterRole + RoleBinding 的模式减少重复定义。
- 定期审计 RBAC 配置，清理不再使用的 Role。

## 参考链接

- [Role - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|Rbac]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/topic-dictionary/security/rolebinding.md|Rolebinding]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrolebinding.md|Clusterrolebinding]]
- [[domain-17-system-foundation/topic-dictionary/security/service-account.md|Service Account]]
