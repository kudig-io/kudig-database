---
title: 准入校验 Webhook
description: ValidatingAdmissionWebhook 是 Kubernetes 准入控制器的扩展机制，允许外部服务拦截 API 请求进行自定义校验，在资源写入
  ...
summary: ValidatingAdmissionWebhook 是 Kubernetes 准入控制器的扩展机制，允许外部服务拦截 API 请求进行自定义校验，在资源写入
  ...
category: dictionary
tags:
- k8s
- glossary
- configuration
- admission
- webhook
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 准入校验 Webhook 是什么
- ValidatingAdmissionWebhook 详解
trigger_keywords:
- 准入校验 Webhook
- ValidatingAdmissionWebhook
- dictionary
prerequisites:
- kubernetes
---



# 准入校验 Webhook（ValidatingAdmissionWebhook）

## 概述

ValidatingAdmissionWebhook 是 Kubernetes 准入控制器的扩展机制，允许外部服务拦截 API 请求进行自定义校验，在资源写入 etcd 前执行策略检查。

## 核心概念/原理

- **准入控制**：拦截 API Server 请求进行校验
- **只读校验**：只验证不修改请求内容
- **外部扩展**：通过 Webhook 调用外部服务
- **失败策略**：Fail（拒绝）/Ignore（放行）

## 关键机制或特性

- ValidatingWebhookConfiguration 注册
- rules 匹配资源类型和操作
- namespaceSelector/objectSelector 过滤范围
- failurePolicy: Fail/Ignore
- sideEffects: None/NoneOnDryRun
- admissionReviewVersions 版本协商
- 超时配置（默认 10s）

## 使用场景与最佳实践

- 自定义策略校验（命名规范/标签要求）
- 安全合规检查（镜像来源/权限级别）
- 成本管控（资源限制验证）
- 与 OPA/Kyverno 集成
- 最佳实践：快速响应（<1s）、Fail 策略要慎重、做好灰度

## 参考链接

- https://kubernetes.io/docs/reference/access-authn-authz/extensible-admission-controllers/
- https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definition-versioning/

## Related

- [[domain-17-system-foundation/topic-dictionary/security/opa.md|OPA]]
- [[domain-17-system-foundation/topic-dictionary/security/kyverno.md|Kyverno]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resource.md|Custom Resource]]
