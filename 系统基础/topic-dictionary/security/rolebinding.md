---
title: 角色绑定
description: RoleBinding 将 Role 或 ClusterRole 的权限授予命名空间内的用户、组或 ServiceAccount。它是 RBAC
  中连接权限定义...
summary: RoleBinding 将 Role 或 ClusterRole 的权限授予命名空间内的用户、组或 ServiceAccount。它是 RBAC
  中连接权限定义...
category: dictionary
tags:
- k8s
- glossary
- rolebinding
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
- 角色绑定 是什么
- RoleBinding 详解
trigger_keywords:
- 角色绑定
- RoleBinding
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 角色绑定

> **英文名**: RoleBinding

## 概述

RoleBinding 将 Role 或 ClusterRole 的权限授予命名空间内的用户、组或 ServiceAccount。它是 RBAC 中连接权限定义和权限主体的桥梁。

## 核心概念/原理

### 核心概念

- **subjects**：权限接收者（User、Group、ServiceAccount）。
- **roleRef**：引用的 Role 或 ClusterRole。
- **命名空间范围**：RoleBinding 仅在创建它的命名空间内生效。

### ClusterRole + RoleBinding 模式

一个常见的模式是使用 ClusterRole 定义权限，然后通过 RoleBinding 限制在特定命名空间内授予：

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: development
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view
subjects:
- kind: ServiceAccount
  name: dev-app
  namespace: development
```

## 关键机制或特性

- RoleBinding 引用 Role 时，权限限制在该命名空间。
- RoleBinding 引用 ClusterRole 时，权限仍限制在 RoleBinding 所在的命名空间。
- 删除 RoleBinding 不会删除关联的 Role/ClusterRole。

## 使用场景与最佳实践

- 为每个应用创建独立的 ServiceAccount 并通过 RoleBinding 授权。
- 避免在 RoleBinding 中引用 `cluster-admin`。
- 定期审计命名空间的 RoleBinding 配置。

## 参考链接

- [RoleBinding - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[系统基础/topic-dictionary/security/rbac.md|Rbac]]
- [[系统基础/topic-dictionary/security/role.md|Role]]
- [[系统基础/topic-dictionary/security/clusterrole.md|Clusterrole]]
- [[系统基础/topic-dictionary/security/clusterrolebinding.md|Clusterrolebinding]]
- [[系统基础/topic-dictionary/security/service-account.md|Service Account]]


<!-- risk-assessed -->
