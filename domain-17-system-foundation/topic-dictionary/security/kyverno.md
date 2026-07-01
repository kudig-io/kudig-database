---
title: Kyverno
description: Kyverno 是 CNCF 孵化项目，专为 Kubernetes 设计的策略引擎。与 OPA Gatekeeper 不同，Kyverno
  使用 YAML 编写...
summary: Kyverno 是 CNCF 孵化项目，专为 Kubernetes 设计的策略引擎。与 OPA Gatekeeper 不同，Kyverno 使用
  YAML 编写...
category: dictionary
tags:
- k8s
- glossary
- kyverno
- policy
- security
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kyverno 是什么
- Kyverno 详解
trigger_keywords:
- Kyverno
- dictionary
prerequisites:
- kubectl-basics
---



# Kyverno

> **英文名**: Kyverno

## 概述

Kyverno 是 CNCF 孵化项目，专为 Kubernetes 设计的策略引擎。与 OPA Gatekeeper 不同，Kyverno 使用 YAML 编写策略，无需学习新语言（如 Rego），降低了策略管理的学习曲线。

## 核心概念/原理

### 核心概念

- **ClusterPolicy**：集群范围的策略。
- **Policy**：命名空间范围的策略。
- **规则类型**：

| 类型 | 功能 |
|------|------|
| Validate | 验证资源是否符合规则 |
| Mutate | 自动修改资源 |
| Generate | 自动生成资源 |
| VerifyImages | 验证容器镜像签名 |

### 与 OPA Gatekeeper 对比

| 特性 | OPA Gatekeeper | Kyverno |
|------|---------------|--------|
| 策略语言 | Rego（DSL） | YAML |
| 学习曲线 | 较高 | 较低 |
| 变更能力 | 仅 Validate | Validate + Mutate + Generate |

## 关键机制或特性

- **Mutate 规则**：自动注入 sidecar、添加默认 labels。
- **Generate 规则**：自动为新命名空间创建 NetworkPolicy/ResourceQuota。
- **Image Verify**：验证镜像的 Sigstore/Cosign 签名。
- **Reports**：生成策略违规报告。
- **Exceptions**：为特定资源定义策略例外。

## 使用场景与最佳实践

- 团队熟悉 YAML 但不想学 Rego 时选择 Kyverno。
- 使用 Mutate 规则自动为所有 Pod 添加安全上下文。
- 使用 Generate 规则自动为新 Namespace 创建默认策略。
- 配合 Sigstore/Cosign 实现镜像签名验证。
- 使用 Kyverno CLI 在 CI 中测试策略。

## 参考链接

- [Kyverno Official](https://kyverno.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/opa.md|OPA]]
- [[domain-17-system-foundation/topic-dictionary/security/admission-controller.md|Admission Controller]]
- [[domain-17-system-foundation/topic-dictionary/security/pod-security-policy.md|Pod Security Policy]]
- [[domain-17-system-foundation/topic-dictionary/security/trivy.md|Trivy]]
- [[domain-17-system-foundation/topic-dictionary/security/webhook.md|Webhook]]
