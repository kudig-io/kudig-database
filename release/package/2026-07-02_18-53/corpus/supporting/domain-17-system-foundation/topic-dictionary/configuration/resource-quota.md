---
title: 资源配额
description: ResourceQuota 是 Kubernetes 命名空间级别的资源配额机制，限制命名空间可使用的计算资源（CPU/Memory）和对象数量（Pod/PVC...
summary: ResourceQuota 是 Kubernetes 命名空间级别的资源配额机制，限制命名空间可使用的计算资源（CPU/Memory）和对象数量（Pod/PVC...
category: dictionary
tags:
- k8s
- glossary
- configuration
- multi-tenancy
- resource-management
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 资源配额 是什么
- ResourceQuota 详解
trigger_keywords:
- 资源配额
- ResourceQuota
- dictionary
prerequisites:
- kubernetes
---



# 资源配额（ResourceQuota）

## 概述

ResourceQuota 是 Kubernetes 命名空间级别的资源配额机制，限制命名空间可使用的计算资源（CPU/Memory）和对象数量（Pod/PVC/Service），是多租户资源治理的核心手段。

## 核心概念/原理

- **命名空间级**：限制每个命名空间的资源总量
- **多维度**：计算资源 + 存储资源 + 对象数量
- **硬限制**：超过配额后请求被拒绝
- **优先级**：不保证公平，先到先得

## 关键机制或特性

- `spec.hard` 定义资源上限
- 计算资源：requests.cpu/memory、limits.cpu/memory
- 存储资源：requests.storage、persistentvolumeclaims
- 对象计数：count/pods、count/services 等
- 作用域：Terminating/NotTerminating/BestEffort/NotBestEffort
- 配额生效延迟（非实时）
- 与 LimitRange 配合使用

## 使用场景与最佳实践

- 多租户的资源隔离和公平分配
- 防止单命名空间耗尽集群资源
- 成本控制和预算管理
- 开发/测试环境的资源限制
- 最佳实践：配合 LimitRange 设默认值、预留缓冲、监控配额使用率

## 参考链接

- https://kubernetes.io/docs/concepts/policy/resource-quotas/
- https://kubernetes.io/docs/concepts/policy/limit-range/

## Related

- [[domain-17-system-foundation/topic-dictionary/configuration/limit-range.md|LimitRange]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace.md|Namespace]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
