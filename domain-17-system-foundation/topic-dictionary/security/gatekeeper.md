---
title: Gatekeeper
description: 'Gatekeeper 是 OPA（Open Policy Agent）的 Kubernetes 原生实现，通过 CRD 在集群中执行准入策略和审计。它将 Reg...'
category: dictionary
tags:
- k8s
- glossary
- gatekeeper
- opa
- policy
- security
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gatekeeper 是什么
- OPA Gatekeeper 详解
trigger_keywords:
- Gatekeeper
- OPA Gatekeeper
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Gatekeeper

> **英文名**: OPA Gatekeeper

## 概述

Gatekeeper 是 OPA（Open Policy Agent）的 Kubernetes 原生实现，通过 CRD 在集群中执行准入策略和审计。它将 Rego 策略封装为 ConstraintTemplate，让非 OPA 专家也能定义和执行策略。

## 核心概念/原理

### 核心资源

| 资源 | 功能 |
|------|------|
| ConstraintTemplate | 参数化的 Rego 策略模板 |
| Constraint | ConstraintTemplate 的实例化（指定参数和目标） |
| Config | 同步 K8s 资源到 OPA 缓存 |

### 执行模式

- **Deny**：拒绝不符合策略的请求（准入控制）。
- **Warn**：允许但生成警告。
- **Dryrun**：仅审计，不阻止。
- **Audit**：定期扫描已有资源的合规性。

## 关键机制或特性

- **Admission Webhook**：拦截 API 请求进行策略检查。
- **Mutation**：自动修改不合规资源（alpha）。
- **External Data**：引用外部数据源辅助策略决策。
- **Library**：社区贡献的 ConstraintTemplate 库。
- 与 CI/CD 集成进行部署前策略检查（gator CLI）。

## 使用场景与最佳实践

- 使用 Gatekeeper 替代 PSP 实施 Pod 安全策略。
- 定义约束：禁止 latest 标签、要求 resource limits、限制特权容器。
- 启用 Audit 定期扫描集群中的违规资源。
- 使用 gator CLI 在 CI 流水线中测试策略合规性。
- 考虑 Kyverno 作为更简单的替代方案（YAML 策略）。

## 参考链接

- [Gatekeeper Official](https://open-policy-agent.github.io/gatekeeper/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/opa.md|OPA]]
- [[domain-17-system-foundation/topic-dictionary/security/kyverno.md|Kyverno]]
- [[domain-17-system-foundation/topic-dictionary/security/admission-controller.md|Admission Controller]]
- [[domain-17-system-foundation/topic-dictionary/security/pod-security-policy.md|Pod Security Policy]]
- [[domain-17-system-foundation/topic-dictionary/security/webhook.md|Webhook]]
