---
title: 基于角色的访问控制
description: RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（Ro...
summary: RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（Ro...
category: dictionary
tags:
- k8s
- glossary
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
- 基于角色的访问控制 是什么
- RBAC (Role-Based Access Control) 详解
trigger_keywords:
- 基于角色的访问控制
- RBAC (Role-Based Access Control)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 基于角色的访问控制

> **英文名**: RBAC (Role-Based Access Control)

## 概述

RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（RoleBinding/ClusterRoleBinding）来控制用户、组和 ServiceAccount 对集群资源的访问权限。

## 核心概念/原理

### RBAC 四大资源

| 资源 | 范围 | 作用 |
|------|------|------|
| Role | 命名空间 | 定义权限规则 |
| ClusterRole | 集群 | 定义权限规则 |
| RoleBinding | 命名空间 | 将权限授予主体 |
| ClusterRoleBinding | 集群 | 将权限授予主体 |

### 授权流程

```
用户请求 → API Server → RBAC Authorizer → 匹配 Role/ClusterRole 规则 → 允许/拒绝
```

### RBAC 决策规则

- **默认拒绝**：没有明确允许的操作都会被拒绝。
- **权限叠加**：多个 RoleBinding 的权限取并集。
- **不可拒绝**：RBAC 只支持"允许"，不支持显式"拒绝"。

## 关键机制或特性

- RBAC 从 K8s v1.8 起成为稳定特性。
- 支持 `*` 通配符匹配所有 verbs/resources/apiGroups。
- 支持自定义动词（如 `bind`、`escalate`）。

## 使用场景与最佳实践

- 始终启用 RBAC（禁用 `--authorization-mode=AlwaysAllow`）。
- 遵循最小权限原则，避免过度授权。
- 使用 `kubectl auth can-i` 验证权限配置。
- 定期运行 RBAC 审计工具（如 rakkess、rbac-lookup）。
- 为每个应用创建独立的 ServiceAccount 并绑定最小权限。

## 参考链接

- [RBAC (Role-Based Access Control) - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/role.md|Role]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/topic-dictionary/security/rolebinding.md|Rolebinding]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrolebinding.md|Clusterrolebinding]]
- [[domain-17-system-foundation/topic-dictionary/security/service-account.md|Service Account]]


<!-- risk-assessed -->
