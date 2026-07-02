---
title: Declarative API
description: '- 声明式 API 与面向终态设计'
summary: '- 声明式 API 与面向终态设计'
category: concepts
tags:
- k8s
- declarative
- api
- design-principle
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Declarative API 是什么
- 如何 Declarative API
trigger_keywords:
- Declarative
- API
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Declarative API

## Core Principle

In Kubernetes, you declare **what** you want, not **how** to achieve it. A YAML manifest describes desired state (replicas, image, ports), and the system continuously works to maintain that state.

## Declarative vs Imperative

| Property | Imperative | Declarative |
|----------|-----------|-------------|
| Approach | "How to do it" | "What it should be" |
| Commands | `kubectl run`, `kubectl scale` | `kubectl apply -f` |
| Idempotency | Not guaranteed | Guaranteed |
| Order sensitivity | Order matters | Order independent |
| State tracking | Manual | System-managed |
| GitOps friendly | No | Yes |

## API Resource Model

Every Kubernetes object follows a standard structure:
- **TypeMeta**: apiVersion + kind (resource type identification)
- **ObjectMeta**: name, namespace, labels, annotations, uid, resourceVersion
- **Spec**: Desired state (user-defined, mutable)
- **Status**: Actual state (system-managed, read-only to users)

Key metadata fields:
- **resourceVersion**: etcd revision number, used for optimistic concurrency control
- **generation**: Incremented each time spec changes
- **ownerReferences**: Enables cascading deletion ([[domain-17-system-foundation/topic-dictionary/fundamentals/garbage-collection.md|garbage collection]])
- **[[Finalizers|finalizers]]**: Pre-delete hooks for resource cleanup

## Server-Side Apply (SSA)

Kubernetes v1.18+ supports Server-Side Apply, which enables multiple controllers to manage different fields of the same object without conflicts. Each field manager owns only the fields they declare, enabling safe collaborative editing.

## Related

- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[concepts/eventual-consistency.md|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/controller-pattern.md|Controller Pattern]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[entities/kube-apiserver.md|kube-apiserver]]
- [[concepts/eventual-consistency.md|Eventual Consistency]]

- 声明式 API 与面向终态设计

<!-- risk-assessed -->
