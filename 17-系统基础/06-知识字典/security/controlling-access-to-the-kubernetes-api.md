---
title: 控制对 Kubernetes API 的访问
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- rbac
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 控制对 Kubernetes API 的访问 是什么
- 如何 控制对 Kubernetes API 的访问
trigger_keywords:
- 控制对
- Kubernetes
- API
- 的访问
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 控制对 [[kubernetes|Kubernetes]]es API|Kubernetes API]] 的访问

## 概述

本页面提供了控制对 Kubernetes API 访问的概览。用户通过 `kubectl`、客户端库或直接发起 REST 请求访问 Kubernetes API。无论是人类用户还是 Kubernetes 服务账号，都可以被授权访问 API。当请求到达 API 时，会依次经历多个阶段。

## 核心概念/原理

Kubernetes API 访问控制分为四个主要阶段：

1. **传输安全（Transport Security）**
2. **认证（Authentication）**
3. **授权（Authorization）**
4. **准入控制（Admission Control）**

### 传输安全

默认情况下，Kubernetes API server 在第一个非 localhost 网络接口的 **6443** 端口上监听，并通过 **TLS** 保护。生产集群通常使用 443 端口。API server 会出示证书，客户端需要信任该证书（如果是私有 CA，则需要在 `~/.kube/config` 中配置 CA 证书）。客户端也可以在此阶段出示 TLS 客户端证书。

### 认证

TLS 建立后，HTTP 请求进入认证步骤。集群管理员配置 API server 运行一个或多个认证模块（Authenticator modules）。常见的认证方式包括：

- 客户端证书
- 密码和普通令牌
- Bootstrap 令牌
- JSON Web Tokens（用于服务账号）

可以配置多个认证模块，按顺序尝试，直到有一个成功。如果请求无法通过认证，将返回 **401** 错误。认证成功后，请求被关联到一个特定的 `username`，部分认证模块还会提供用户所属的 `group` 信息。

Kubernetes 使用用户名进行访问控制决策和请求日志记录，但 API 中**没有 User 对象**，也不存储用户信息。

### 授权

认证通过后，请求必须被授权。授权需要知道：请求者用户名、请求动作、受影响的对象。如果现有策略声明用户有权完成请求的动作，则请求被授权。

Kubernetes 支持多种授权模块：

- **ABAC 模式**
- **RBAC 模式**
- **Webhook 模式**

如果配置了多个授权模块，Kubernetes 会依次检查，**只要有一个模块授权通过**，请求即可继续。如果所有模块都拒绝，则返回 **403** 错误。

### 准入控制

准入控制器是可以修改或拒绝请求的软件模块。与授权模块相比，准入控制器还能访问被创建或修改对象的内容。准入控制器作用于创建、修改、删除或连接（proxy）对象的请求，**不作用于只读请求**。

多个准入控制器按顺序调用，**与认证和授权不同，如果任何一个准入控制器拒绝，请求会立即被拒绝**。准入控制器还可以为字段设置复杂的默认值。

### API 验证与持久化

通过所有准入控制器后，请求会经过对应 API 对象的验证例程，然后写入对象存储（[[17-系统基础/06-知识字典/fundamentals/etcd.md|etcd]]）。

### 审计

Kubernetes 审计提供了一组按时间顺序排列的安全相关记录，记录集群中的操作序列。更多详情请参阅 Auditing 文档。

## 关键机制或特性

- **TLS 保护**：确保 API server 与客户端、节点之间的通信加密。
- **多认证模块**：支持灵活的身份验证集成。
- **多授权模块**：允许与组织级或云提供商的访问控制系统集成。
- **准入控制器链**：在对象持久化之前进行最后的策略检查和修改。
- **审计日志**：记录安全相关操作，便于事后分析和合规审查。

## 使用场景

- 保护生产集群 API 免受未授权访问。
- 集成企业现有的身份认证系统（如 LDAP、Active Directory、OIDC）。
- 实施细粒度的访问控制策略（RBAC/ABAC）。
- 在对象创建前强制执行安全、合规和资源配置策略（通过准入控制器）。

## 最佳实践/注意事项

- 生产环境中使用受信任的 CA 签发的证书，避免使用自签名证书。
- 优先使用 **RBAC** 进行授权管理，ABAC 已不推荐用于新集群。
- 正确配置准入控制器链，确保关键的安全准入控制器（如 PodSecurity、NodeRestriction）已启用。
- 启用并定期检查 **审计日志**，监控异常访问模式。
- 限制 API server 的网络暴露范围，避免直接暴露在互联网上。

## 参考链接

- https://kubernetes.io/docs/concepts/security/controlling-access/

## Related
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
