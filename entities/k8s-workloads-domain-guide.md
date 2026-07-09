---
title: Kubernetes Workloads Domain Guide
description: '- [[concepts/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
summary: '- [[concepts/Deployment × Secret 管理.md|Deployment × Secret 管理]]'
category: references
tags:
- k8s
- workloads
- 工作负载
- pod
- deployment
- statefulset
- reference
- daemonset
- job
- cronjob
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Workloads Domain Guide 是什么
- 如何 Kubernetes Workloads Domain Guide
trigger_keywords:
- Kubernetes
- Workloads
- Domain
- Guide
prerequisites:
- kubectl-basics
- pod-lifecycle
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Workloads Domain Guide

## Source

Distilled from 工作负载 (24 documents, Kubernetes v1.28-v1.32).

## Workload Controllers

| Controller | Manages | Update Strategy | Use Case |
|-----------|---------|----------------|----------|
| **[[deployment]]** | ReplicaSet | RollingUpdate, Recreate | Stateless microservices |
| **StatefulSet** | Pod (ordered) | RollingUpdate (reverse), Partition | Databases, message brokers |
| **DaemonSet** | Pod (per node) | RollingUpdate | Logging, monitoring, CNI agents |
| **Job** | Pod (run-to-completion) | - | Batch processing, data migration |
| **CronJob** | Job (scheduled) | - | Backup, cleanup, periodic tasks |

## Pod Lifecycle Phases

Pending -> Running -> Succeeded/Failed -> Terminating

Conditions: PodScheduled, Initialized, ContainersReady, Ready.

## Production Patterns

- Set `revisionHistoryLimit: 10` for rollback capability
- Use `maxSurge: 1, maxUnavailable: 0` for zero-downtime updates
- Configure PodAntiAffinity for replica distribution
- Always set resource requests/limits
- Use three probes: startup, liveness, readiness

## Sidecar Pattern

v1.28+ native sidecar containers: init containers with `restartPolicy: Always` run alongside main containers, enabling service mesh proxies, log shippers, and config watchers without external injection.

## Related

- [[reference|#reference Hub]] — tag hub

- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[pod-lifecycle]] — Pod Lifecycle
- [[pod-lifecycle|Pod Lifecycle]]
- [[deployment|Deployment]]
- [[entities/statefulset.md|StatefulSet]]
- [[skills/configure-health-probes.md|Configure Health Probes]]

- [[concepts/Deployment × Secret 管理.md|Deployment × Secret 管理]]

<!-- risk-assessed -->
