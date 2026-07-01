---
title: 准入控制器
description: 'Admission Controller 是 Kubernetes API Server 中的插件机制，在对象持久化之前拦截和处理 API 请求。它可以验证和修...'
category: dictionary
tags:
- k8s
- glossary
- security
- admission
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 准入控制器 是什么
- Admission Controller 详解
trigger_keywords:
- 准入控制器
- Admission Controller
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 准入控制器

> **英文名**: Admission Controller

## 概述

Admission Controller 是 Kubernetes API Server 中的插件机制，在对象持久化之前拦截和处理 API 请求。它可以验证和修改请求中的对象，是实施集群策略和安全控制的关键组件。

## 核心概念/原理

### 类型

- **Validating（验证型）**：只验证请求是否合规，不修改对象。如 `ValidatingAdmissionWebhook`。
- **Mutating（变更型）**：可以修改请求中的对象。如 `MutatingAdmissionWebhook`。

### 内置准入控制器

- **LimitRanger**：检查资源是否超出 LimitRange。
- **ResourceQuota**：检查资源是否超出 ResourceQuota。
- **PodSecurity**：强制执行 Pod 安全标准（替代 PSP）。
- **NodeRestriction**：限制 kubelet 可以修改的 API 对象。
- **AlwaysPullImages**：强制每次都拉取镜像。

## 关键机制或特性

- 准入控制链：Mutating → Object Validation → Validating。
- Mutating 控制器可以修改对象，可能需要多次执行（收敛）。
- Webhook 超时或失败时，`failurePolicy` 决定是拒绝（Fail）还是允许（Ignore）。

## 使用场景与最佳实践

- 使用 `ValidatingAdmissionWebhook` 实施自定义策略（如镜像白名单）。
- 使用 OPA Gatekeeper 或 Kyverno 实现声明式策略管理。
- 配置 Webhook 的 `failurePolicy: Ignore` 避免 Webhook 故障导致集群不可用。
- 为 Webhook 配置 `namespaceSelector` 排除系统命名空间。

## 参考链接

- [Admission Controller - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/admission-controllers/)

## Related

[[domain-17-system-foundation/topic-dictionary/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
