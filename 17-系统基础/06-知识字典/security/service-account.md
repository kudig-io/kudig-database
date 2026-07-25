---
title: 服务账号
description: ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount
  向 API Serv...
summary: ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount 向 API
  Serv...
category: dictionary
tags:
- k8s
- glossary
- service-account
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
- 服务账号 是什么
- ServiceAccount 详解
trigger_keywords:
- 服务账号
- ServiceAccount
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务账号

> **英文名**: ServiceAccount

## 概述

ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount 向 API Server 认证身份，获取访问集群资源的权限。

## 核心概念/原理

### 核心概念

- **默认 ServiceAccount**：每个命名空间自动创建 `default` ServiceAccount。
- **Token 注入**：kubelet 自动将 ServiceAccount Token 挂载到 Pod 中（Projected Volume）。
- **Token 特性**：
  - 有界的（bound to Pod）。
  - 有过期时间（默认 1 小时，自动轮转）。
  - 观众限制（audience-restricted）。

### RBAC 集成

通过 RoleBinding 或 ClusterRoleBinding 将权限授予 ServiceAccount，实现 Pod 级别的权限控制。

## 关键机制或特性

- Token Request API（v1.20+）提供时间有界、受众受限的 Token。
- `automountServiceAccountToken: false` 可以禁止自动挂载 Token。
- `boundServiceAccountTokenVolume` 特性确保 Token 安全。

## 使用场景与最佳实践

- 为每个应用创建独立的 ServiceAccount，避免使用 default。
- 遵循最小权限原则，只授予必要的 RBAC 权限。
- 对不需要 API 访问的 Pod，禁用自动 Token 挂载。
- 使用 TokenRequest API 为外部服务生成短期 Token。

## 参考链接

- [ServiceAccount - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/service-accounts-admin/)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
