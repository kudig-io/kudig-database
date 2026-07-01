---
title: Webhook
description: 'Webhook 是 Kubernetes 中允许外部服务介入 API 请求处理流程的回调机制。通过 Webhook，可以将认证、授权和准入控制逻辑委托给外部服务...'
category: dictionary
tags:
- k8s
- glossary
- security
- webhook
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Webhook 是什么
- Webhook 详解
trigger_keywords:
- Webhook
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Webhook

> **英文名**: Webhook

## 概述

Webhook 是 Kubernetes 中允许外部服务介入 API 请求处理流程的回调机制。通过 Webhook，可以将认证、授权和准入控制逻辑委托给外部服务。

## 核心概念/原理

### Webhook 类型

- **MutatingAdmissionWebhook**：在对象持久化前修改对象。
- **ValidatingAdmissionWebhook**：在对象持久化前验证对象。
- **Authentication Webhook**：自定义认证逻辑（Token Review）。
- **Authorization Webhook**：自定义授权逻辑（SubjectAccessReview）。

### 工作原理

```
API Request → API Server → Webhook (HTTPS) → External Service → Response
```

## 关键机制或特性

- Webhook 服务需要通过 TLS 加密通信。
- 支持 `caBundle` 或 `service` 引用配置 Webhook 服务。
- Webhook 的性能直接影响 API Server 的请求延迟。

## 使用场景与最佳实践

- 实现 Webhook 时确保低延迟和高可用。
- 配置合理的超时时间（默认 10 秒）。
- 使用 `namespaceSelector` 限制 Webhook 的作用范围。
- 测试 Webhook 的故障场景（超时/不可达）。

## 参考链接

- [Webhook - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/webhook/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|Rbac]]
- [[domain-17-system-foundation/topic-dictionary/security/role.md|Role]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/topic-dictionary/security/rolebinding.md|Rolebinding]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrolebinding.md|Clusterrolebinding]]
