---
title: 服务账号
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- rbac
- webhook
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务账号 是什么
- 如何 服务账号
trigger_keywords:
- 服务账号
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# 服务账号

## 概述

ServiceAccount（服务账号）是 [[Kubernetes|Kubernetes]] 中的一种非人类账户，用于在集群内提供独立的安全身份。应用 Pod、系统组件以及集群内外的实体都可以使用特定 ServiceAccount 的凭据来标识自己。该身份在多种场景下非常有用，例如向 API server 认证或实施基于身份的安全策略。

## 核心概念/原理

ServiceAccount 具有以下关键属性：

- **命名空间范围（Namespaced）**：每个 ServiceAccount 都绑定到一个 Kubernetes 命名空间。每个命名空间在创建时会自动获得一个名为 `default` 的 ServiceAccount。
- **轻量级（Lightweight）**：ServiceAccount 存在于集群中，通过 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] 定义，可快速创建以支持特定任务。
- **可移植（Portable）**：复杂的容器化工作负载配置可以包含系统组件的 ServiceAccount 定义，其轻量级和命名空间特性使配置易于移植。

与用户账户（User account）不同：

| 描述 | ServiceAccount | 用户或组 |
|------|----------------|----------|
| 位置 | Kubernetes API（ServiceAccount 对象） | 外部系统 |
| 访问控制 | Kubernetes RBAC 或其他授权机制 | Kubernetes RBAC 或其他 IAM 机制 |
| 预期用途 | 工作负载、自动化 | 人员 |

### 默认服务账号

Kubernetes 会自动为每个命名空间创建一个名为 `default` 的 ServiceAccount。如果删除该对象，控制平面会自动重新创建。如果在部署 Pod 时未手动指定 ServiceAccount，Kubernetes 会自动分配该命名空间的 `default` ServiceAccount。

## 关键机制或特性

### 权限授予

使用 Kubernetes 内置的 **RBAC** 机制为 ServiceAccount 授予最小所需权限：

- 创建 Role（定义权限）和 RoleBinding（将 Role 绑定到 ServiceAccount）。
- 支持跨命名空间访问：可以在目标命名空间中创建 RoleBinding，绑定其他命名空间中的 ServiceAccount。

### 为 Pod 分配 ServiceAccount

在 Pod 规范中设置 `spec.serviceAccountName` 字段即可分配 ServiceAccount。Kubernetes 会自动向 Pod 提供该账号的凭据。

自 Kubernetes v1.22 起，Kubernetes 使用 **TokenRequest API** 获取短期、自动轮换的令牌，并以 **projected volume** 的形式挂载到 Pod 中。

若要阻止自动注入凭据，可在 Pod 规范中设置：

```yaml
automountServiceAccountToken: false
```

### 凭据获取方式

- **TokenRequest API（推荐）**：从应用代码内请求短期 ServiceAccount 令牌，令牌到期自动失效并支持轮换。可为外部应用使用 sidecar 容器获取令牌。
- **Token Volume Projection（推荐）**：在 Pod 规范中配置 projected volume，[[kubelet|kubelet]] 会自动将 ServiceAccount 令牌作为 projected volume 挂载，并在过期前自动轮换。
- **ServiceAccount Token Secrets（不推荐）**：将 ServiceAccount 令牌作为 Kubernetes Secret 挂载。这些令牌不会过期也不会轮换。自 v1.24 起默认不再自动生成此类 Secret。

### 认证流程

ServiceAccount 使用签名的 **JSON Web Token（JWT）** 向 Kubernetes API server 认证。API server 会依次验证：

1. 令牌签名
2. 令牌是否过期
3. 令牌声明中的对象引用是否仍然有效
4. 令牌当前是否有效
5. Audience 声明

对于通过 TokenRequest API 发放的**绑定令牌（bound tokens）**，API server 还会验证使用该 ServiceAccount 的特定对象（如 Pod）是否仍然存在（按对象唯一 ID 匹配）。

### 验证 ServiceAccount 凭据（自定义服务）

- **TokenReview API（推荐）**：可立即使绑定到 API 对象（Pod、Secret、Node 等）的令牌失效。例如删除包含 projected token 的 Pod 后，TokenReview 会立即失败。
- **OIDC discovery**：客户端在令牌过期前会一直认为其有效。

## 使用场景

- Pod 需要与 Kubernetes API server 通信（如读取 Secret、跨命名空间访问 Lease 对象）。
- Pod 需要与外部服务通信（如商业云 API），且提供商支持配置基于 ServiceAccount 的信任关系。
- 使用 `imagePullSecret` 向私有镜像仓库认证。
- 外部服务（如 CI/CD 流水线）需要向 Kubernetes API server 认证。
- 第三方安全软件依赖 ServiceAccount 身份对 Pod 进行分组和策略管理。

## 最佳实践/注意事项

- 遵循**最小权限原则**，为每个 ServiceAccount 仅授予完成任务所需的最小 RBAC 权限。
- **避免使用 `default` ServiceAccount**；为每个工作负载或微服务创建独立的 ServiceAccount。
- 对于不需要访问 Kubernetes API 的 Pod，设置 `automountServiceAccountToken: false`。
- **避免创建长期有效的 ServiceAccount 令牌 Secret**，优先使用 TokenRequest API 或 Token Volume Projection 获取短期、自动轮换的令牌。
- 外部应用尽量不要使用长期 bearer token，可考虑使用受保护的私钥和证书，或自定义认证 Webhook。
- 自 v1.32 起，`kubernetes.io/enforce-mountable-secrets` 注解已被弃用，建议使用独立命名空间来隔离对挂载 Secret 的访问。

## 参考链接

- https://kubernetes.io/docs/concepts/security/service-accounts/

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
