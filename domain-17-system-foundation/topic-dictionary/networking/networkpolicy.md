---
title: 网络策略
description: 'NetworkPolicy 是 Kubernetes 中控制 Pod 之间以及 Pod 与外部网络之间流量的资源对象。它采用默认允许（default allow...'
category: dictionary
tags:
- k8s
- glossary
- networkpolicy
- security
- cni
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络策略 是什么
- NetworkPolicy 详解
trigger_keywords:
- 网络策略
- NetworkPolicy
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 网络策略

> **英文名**: NetworkPolicy

## 概述

NetworkPolicy 是 Kubernetes 中控制 Pod 之间以及 Pod 与外部网络之间流量的资源对象。它采用默认允许（default allow）模型，通过定义 ingress 和 egress 规则实现网络隔离。

## 核心概念/原理

### 核心概念

- **Pod Selector**：选择策略作用的目标 Pod。
- **Ingress 规则**：控制入站流量（谁可以访问目标 Pod）。
- **Egress 规则**：控制出站流量（目标 Pod 可以访问谁）。
- **Policy Types**：指定 `Ingress`、`Egress` 或两者。

### 默认行为

| 场景 | 行为 |
|------|------|
| 无 NetworkPolicy | 允许所有流量 |
| 仅有 Ingress 策略 | 入站受限，出站不受限 |
| 同时有 Ingress + Egress | 双向受限 |

## 关键机制或特性

- NetworkPolicy 需要 CNI 插件支持（Calico、Cilium、Weave 等）。
- 不支持的 CNI 会静默忽略 NetworkPolicy 资源。
- 规则中的 `namespaceSelector` 和 `podSelector` 可以组合使用。
- `ipBlock` 支持 CIDR 匹配（除 `except` 子网外）。

## 使用场景与最佳实践

- 为每个命名空间创建默认 deny-all 策略，再按需放行。
- 使用标签选择器精确控制流量，避免过度宽松的策略。
- 定期审计 NetworkPolicy 覆盖情况，确保无遗漏。
- 生产环境建议配合 Cilium 或 Calico 的高级网络策略功能。

## 参考链接

- [NetworkPolicy - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/network-policies/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/cni.md|CNI]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/topic-dictionary/security/security-context.md|Security Context]]
