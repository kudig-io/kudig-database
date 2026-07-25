---
title: 集群角色绑定
description: ClusterRoleBinding 将 ClusterRole 的权限授予集群范围的主体。与 RoleBinding 不同，ClusterRoleBindin...
summary: ClusterRoleBinding 将 ClusterRole 的权限授予集群范围的主体。与 RoleBinding 不同，ClusterRoleBindin...
category: dictionary
tags:
- k8s
- glossary
- clusterrolebinding
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
- 集群角色绑定 是什么
- ClusterRoleBinding 详解
trigger_keywords:
- 集群角色绑定
- ClusterRoleBinding
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群角色绑定

> **英文名**: ClusterRoleBinding

## 概述

ClusterRoleBinding 将 ClusterRole 的权限授予集群范围的主体。与 RoleBinding 不同，ClusterRoleBinding 的权限在整个集群内生效。

## 核心概念/原理

### 核心概念

- **集群范围生效**：授权主体可以在所有命名空间执行授权的操作。
- **主体类型**：User、Group、ServiceAccount。
- **使用场景**：集群管理员权限、跨命名空间权限、集群级资源访问。

### 示例

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-readers
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: developers
```

## 关键机制或特性

- ClusterRoleBinding 一旦创建，授权主体在整个集群范围内拥有相应权限。
- 谨慎使用 ClusterRoleBinding，遵循最小权限原则。
- 删除 ClusterRoleBinding 不影响关联的 ClusterRole。

## 使用场景与最佳实践

- 仅对需要集群范围权限的场景使用 ClusterRoleBinding。
- 优先使用 Group 而非单独 User 来管理集群级权限。
- 定期使用 `kubectl get clusterrolebindings` 审计集群级授权。

## 参考链接

- [ClusterRoleBinding - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[domain-17-system-foundation/知识字典/security/rbac.md|Rbac]]
- [[domain-17-system-foundation/知识字典/security/role.md|Role]]
- [[domain-17-system-foundation/知识字典/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/知识字典/security/service-account.md|Service Account]]
- [[domain-17-system-foundation/知识字典/security/service-account-token.md|Service Account Token]]


<!-- risk-assessed -->
