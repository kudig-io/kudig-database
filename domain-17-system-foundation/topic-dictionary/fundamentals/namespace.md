---
title: 命名空间
description: 'Namespace 是 Kubernetes 的逻辑隔离机制，将集群资源划分为虚拟的子集群，实现多团队/多环境/多租户的资源隔离和访问控制。...'
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- multi-tenancy
- isolation
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 命名空间 是什么
- Namespace 详解
trigger_keywords:
- 命名空间
- Namespace
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# 命名空间（Namespace）

## 概述

Namespace 是 Kubernetes 的逻辑隔离机制，将集群资源划分为虚拟的子集群，实现多团队/多环境/多租户的资源隔离和访问控制。

## 核心概念/原理

- **逻辑隔离**：同一集群内的资源分组
- **资源配额**：限制每个命名空间的资源用量
- **RBAC 边界**：基于命名空间的访问控制
- **默认命名空间**：default/kube-system/kube-public/kube-node-lease

## 关键机制或特性

- Namespace 隔离资源名称（同命名空间内唯一）
- ResourceQuota 限制 CPU/Memory/PVC/对象数量
- LimitRange 设置默认的资源请求和限制
- NetworkPolicy 控制跨命名空间网络访问
- RBAC Role/RoleBinding 限定命名空间权限
- 集群级资源（Node/PV/ClusterRole）不受命名空间约束
- 4 个系统命名空间有特定用途

## 使用场景与最佳实践

- 多团队/多项目的资源隔离
- 开发/测试/生产环境分离
- 资源配额和成本分摊
- 最小权限的 RBAC 设计
- 最佳实践：避免 default 命名空间、命名规范、配合 NetworkPolicy

## 参考链接

- https://kubernetes.io/docs/concepts/overview/working-with-objects/namespaces/
- https://kubernetes.io/docs/concepts/policy/resource-quotas/

## Related

- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/networking/networkpolicy.md|NetworkPolicy]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster.md|Cluster]]
