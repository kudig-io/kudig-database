---
title: Controller Pattern (Reconciliation Loop)
description: Controller Pattern (Reconciliation Loop) — Kubernetes 生产运维知识库
summary: Controller Pattern (Reconciliation Loop) — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- controller
- reconciliation
- design-pattern
- etcd
- hpa
- statefulset
- daemonset
- job
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Controller Pattern (Reconciliation Loop) 是什么
- 如何 Controller Pattern (Reconciliation Loop)
trigger_keywords:
- Controller
- Pattern
- Reconciliation
- Loop
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Controller Pattern (Reconciliation Loop)

## Core Mechanism

The controller pattern is Kubernetes's fundamental automation mechanism. Every controller follows the same reconciliation loop:

1. **Observe**: Use Informer to Watch for resource changes and cache them locally
2. **Compare**: Diff desired state (Spec) against actual state (Status)
3. **Act**: Take corrective action to reduce the gap
4. **Update**: Write the new Status back to the API Server
5. **Repeat**: Wait for the next event or periodic re-sync

## Informer + Workqueue Architecture

Controllers use the **Informer pattern** for efficient state observation:
- **Informer**: Maintains a local cache, handles List+Watch, triggers event handlers
- **Indexer**: Provides fast lookup by labels, namespaces, or custom keys
- **Workqueue**: Decouples event detection from processing; supports rate-limited retries with exponential backoff

This architecture ensures controllers are resilient to API Server outages and network partitions.

## Built-in Controllers

| Controller | Observes | Manages | Purpose |
|-----------|----------|---------|---------|
| Deployment Controller | Deployment | [[ReplicaSet|ReplicaSet]] | Rolling updates, rollback |
| ReplicaSet Controller | ReplicaSet | Pod | Maintain replica count |
| [[StatefulSet|StatefulSet]] Controller | StatefulSet | Pod, PVC | Ordered stateful management |
| [[DaemonSet|DaemonSet]] Controller | DaemonSet, Node | Pod | One Pod per node |
| Job Controller | Job | Pod | Run-to-completion tasks |
| Node Controller | Node | Pod (eviction) | Node health monitoring |
| PV Controller | PV, PVC | PV, PVC | Volume binding |
| HPA Controller | HPA, metrics | Deployment | Horizontal autoscaling |

## Key Properties

- **Idempotent**: Running reconciliation multiple times produces the same result
- **Eventually Consistent**: System converges to desired state over time
- **Fault Tolerant**: Controller restart does not lose state (state lives in etcd)
- **Non-blocking**: Errors are re-queued with backoff, not blocking other reconciliations

## Related
- [[concepts/控制器模式 × 可观测性.md|控制器模式 × 可观测性]] — 综合
- [[concepts/控制器模式 × Deployment.md|控制器模式 × Deployment]] — 综合

- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/eventual-consistency.md|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/declarative-api.md|Declarative API]]
- [[concepts/watch-mechanism.md|Watch Mechanism]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[operator-pattern|Operator Pattern]]
- [[concepts/eventual-consistency.md|Eventual Consistency]]

- 控制器模式与调谐循环
- [[entities/metal3-io.md|Metal3]] — Cross-reference


<!-- risk-assessed -->
