---
title: 服务账号令牌
description: ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server
  证明身份。从 K8s...
summary: ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server 证明身份。从
  K8s...
category: dictionary
tags:
- k8s
- glossary
- security
- service-account
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务账号令牌 是什么
- ServiceAccount Token 详解
trigger_keywords:
- 服务账号令牌
- ServiceAccount Token
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务账号令牌

> **英文名**: ServiceAccount Token

## 概述

ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server 证明身份。从 K8s v1.21 起使用 TokenRequest API 颁发有界、过期的 Token。

## 核心概念/原理

### Token 特性

- **有界（Bound）**：Token 绑定到特定的 Pod 和 ServiceAccount。
- **过期（Expiring）**：默认 1 小时过期，kubelet 自动轮转。
- **受众限制（Audience-restricted）**：Token 只能用于特定的 API 受众。

### Token 注入

kubelet 通过 Projected Volume 自动将 Token 注入 Pod：

```yaml
# 自动注入（无需手动配置）
volumes:
- name: kube-api-access
  projected:
    sources:
    - serviceAccountToken:
        expirationSeconds: 3600
        path: token
```

## 关键机制或特性

- 旧版 Secret-based Token（非过期）已弃用。
- TokenRequest API 提供短期、有界的 Token。
- `automountServiceAccountToken: false` 可以禁用自动 Token 注入。

## 使用场景与最佳实践

- 不需要 API 访问的 Pod 禁用自动 Token 挂载。
- 使用 TokenRequest API 为外部服务生成短期 Token。
- 审计 ServiceAccount Token 的使用情况。

## 参考链接

- [ServiceAccount Token - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/service-accounts-admin/#tokenrequest-api)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
