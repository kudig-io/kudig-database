---
title: 加固指南 - 认证机制
description: '# 加固指南 - 认证机制'
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
- 加固指南 - 认证机制 是什么
- 如何 加固指南 - 认证机制
trigger_keywords:
- 加固指南
- 认证机制
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# 加固指南 - 认证机制

## 概述

选择适当的认证机制是保护集群安全的关键方面。[[entities/kubernetes|[[Kubernetes|kubernetes]]]] 提供了多种内置认证机制，每种机制都有其自身的优缺点，在选择最佳认证方案时需要仔细权衡。通常建议启用尽可能少的认证机制，以简化用户管理并防止用户保留不再需要的集群访问权限。需要注意的是，Kubernetes 集群内部没有内置的用户数据库，而是从配置的认证系统中获取用户信息并用于授权决策。

## 核心概念/原理

对于具有多个用户直接访问 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api|Kubernetes API]] 的生产集群，**建议使用外部认证源（如 OIDC）**。内部认证机制（如下文所述的客户端证书和服务账号令牌）不适合人类用户的生产用例。

## 关键机制或特性

### X.509 客户端证书认证

Kubernetes 利用 X.509 客户端证书进行系统组件认证（如 [[kubelet|kubelet]] 向 API Server 认证）。虽然也可用于用户认证，但由于以下限制，可能不适合生产环境：

- **无法单独撤销**：证书一旦泄露，攻击者可一直使用到其过期。建议为客户端证书配置短有效期。
- **重新签发 CA 风险**：如需使证书失效，必须重新签发证书颁发机构（CA），可能引入可用性风险。
- **无永久记录**：集群中不会永久记录已签发的客户端证书，若需跟踪必须自行记录。
- **私钥无法密码保护**：任何能读取私钥文件的人都可以使用它。
- **网络架构要求**：需要客户端与 API server 之间的直接连接，不能 intervening TLS termination。
- **组信息固定**：组信息嵌入在客户端证书的 `O` 值中，证书有效期内无法更改用户的组成员身份。

### 静态令牌文件

虽然 Kubernetes 允许从控制平面节点磁盘上的静态令牌文件加载凭据，但**不推荐用于生产服务器**：

- 凭据以明文形式存储在控制平面节点磁盘上。
- 更改任何凭据都需要重启 API server 进程。
- 没有凭据轮换机制，必须由管理员手动修改和分发。
- 没有锁定机制防止暴力破解。

### Bootstrap 令牌

Bootstrap 令牌用于**节点加入集群**，不推荐用于用户认证：

- 具有硬编码的组成员身份，不适合通用认证。
- 手动生成的令牌可能过弱，容易被猜测。
- 没有锁定机制防止暴力破解。

### ServiceAccount Secret 令牌

ServiceAccount Secret 令牌允许集群内的工作负载向 API server 认证。在 Kubernetes < 1.23 时这是默认选项，但正被 TokenRequest API 令牌取代。虽然它们也可用于用户认证，但通常不适合：

- 无法设置过期时间，直到关联的 ServiceAccount 被删除前一直有效。
- 任何能在命名空间中读取 Secret 的集群用户都能看到认证令牌。
- ServiceAccount 不能被添加到任意组，使 RBAC 管理复杂化。

### TokenRequest API 令牌

TokenRequest API 是生成短期凭据的有用工具，用于服务向 API server 或第三方系统认证。但**一般不推荐用于用户认证**，因为缺少撤销方法，且安全地分发凭据给用户具有挑战性。

使用时建议实施**短有效期**，以降低令牌泄露后的影响。

### OpenID Connect（OIDC）令牌认证

Kubernetes 支持使用 **OpenID Connect（OIDC）** 将外部认证服务与 Kubernetes API 集成。市场上有多种软件可用于此集成。使用时需考虑以下加固措施：

- 集群中支持 OIDC 认证的软件应与通用工作负载隔离，因为它会以高权限运行。
- 某些托管 Kubernetes 服务对可用的 OIDC 提供商有限制。
- OIDC 令牌应具有短有效期，以降低泄露影响。

### Webhook 令牌认证

Webhook 令牌认证允许通过 webhook 联系内部或外部的认证服务进行认证决策。其适用性取决于所使用的认证服务软件。配置时需要注意：

- 配置 Webhook 认证需要访问控制平面服务器的文件系统，这在托管 Kubernetes 上通常不可行（除非提供商特别支持）。
- 支持此功能的软件应以高权限运行，应与通用工作负载隔离。

### 认证代理（Authenticating Proxy）

认证代理是另一种集成外部认证系统的方式。Kubernetes 期望从代理接收带有特定标头的请求，以指示用户名和组成员身份。使用时需注意：

- 代理与 Kubernetes API server 之间必须使用安全配置的 TLS，以防范流量拦截或嗅探攻击。
- 攻击者如果能够修改请求标头，可能获得对 Kubernetes 资源的未授权访问。因此必须确保标头安全且不可篡改。

## 使用场景

- **系统组件间通信**：使用 X.509 客户端证书（如 kubelet、kube-proxy）。
- **节点加入集群**：使用 Bootstrap 令牌。
- **集群内工作负载认证**：使用 TokenRequest API 或 Token Volume Projection。
- **多用户生产集群**：使用 OIDC 或认证代理/Webhook 集成企业身份提供商。

## 最佳实践/注意事项

- 生产环境中为人类用户优先选择 **OIDC** 或 **Webhook/认证代理** 等外部认证机制。
- 尽量启用**最少数量**的认证机制，简化管理和审计。
- 为所有短期凭据（TokenRequest、OIDC 令牌）配置**短有效期**。
- **避免**将 Bootstrap 令牌、静态令牌文件和长期 ServiceAccount Secret 令牌用于用户认证。
- 如需审计用户访问，需审查**所有**已配置认证来源的凭据记录。
- 配置 OIDC 或 Webhook 支持组件时，将其与通用工作负载隔离运行。

## 参考链接

- https://kubernetes.io/docs/concepts/security/hardening-guide/authentication-mechanisms/
