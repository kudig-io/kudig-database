---
title: kube-scheduler
description: kube-scheduler — Kubernetes 生产运维知识库
summary: kube-scheduler — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- scheduler
- control-plane
- scheduling
- algorithm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-scheduler 是什么
- 如何 kube-scheduler
trigger_keywords:
- kube-scheduler
prerequisites:
- kubectl-basics
---



# kube-scheduler

## Role

kube-scheduler watches for unscheduled [[Pods|Pods]] and assigns each to the most suitable node. It is the only component that writes to `Pod.spec.nodeName`.

## [[domain-17-system-foundation/topic-dictionary/scheduling/scheduling-framework.md|Scheduling Framework]]

The scheduler is plugin-based with extension points:

| Phase | Extension Point | Purpose |
|-------|----------------|---------|
| PreFilter | Plugin | Fast pre-checks |
| Filter | Plugin | Node feasibility |
| PostFilter | Plugin | Preemption |
| PreScore | Plugin | [[Score|Score]] preparation |
| Score | Plugin | Node ranking |
| Reserve | Plugin | Resource reservation |
| Permit | Plugin | Binding approval |
| PreBind | Plugin | Pre-bind actions |
| Bind | Plugin | Node assignment |
| PostBind | Plugin | Post-bind cleanup |

## Default Plugins

Key default plugins include NodeResourcesFit, NodeAffinity, TaintToleration, InterPodAffinity, PodTopologySpread, VolumeBinding, ImageLocality, and NodeResourcesBalancedAllocation.

## Configuration

Custom scheduler configurations can adjust plugin weights, enable/disable plugins, and set scheduling profiles for different workload classes. Multiple profiles can coexist, and Pods select a profile via `schedulerName`.

## HA

Multiple scheduler instances run with leader election. Only the leader schedules; standby instances take over if leader fails (lease duration 15s, renew deadline 10s).

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[concepts/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[concepts/scheduling-algorithm.md|Scheduling Algorithm]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[concepts/resource-management.md|Resource Management]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]

- 20-kube-scheduler-deep-dive