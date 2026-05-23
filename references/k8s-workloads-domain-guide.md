---
title: Kubernetes Workloads Domain Guide
description: '- [[Deployment × Secret 管理]]'
category: references
tags:
- k8s
- workloads
- domain-02-workloads-applications
- pod
- deployment
- statefulset
- reference
- daemonset
- job
- cronjob
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
created: "2026-05-23"
---

# Kubernetes Workloads Domain Guide

## Source

Distilled from domain-02-workloads-applications (24 documents, Kubernetes v1.28-v1.32).

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

- [[entities/statefulset|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[pod-lifecycle]] — Pod Lifecycle
- [[pod-lifecycle|Pod Lifecycle]]
- [[deployment|Deployment]]
- [[entities/statefulset|StatefulSet]]
- [[skills/configure-health-probes|Configure Health Probes]]

- [[Deployment × Secret 管理]]