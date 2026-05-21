---
title: Resource Management (Requests, Limits, QoS)
description: Resource Management (Requests, Limits, QoS) — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- resources
- qos
- requests
- limits
- eviction
- cgroups
- kubelet
- scheduler
- vpa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Resource Management (Requests, Limits, QoS) 是什么
- 如何 Resource Management (Requests, Limits, QoS)
trigger_keywords:
- Resource
- Management
- Requests
- Limits
- QoS
prerequisites:
- kubectl-basics
---

# Resource Management (Requests, Limits, QoS)

## Requests vs Limits

| Field | Purpose | Scheduling | Runtime |
|-------|---------|------------|---------|
| **requests** | Minimum guaranteed resources | Used by scheduler to find fitting nodes | cgroup guaranteed share |
| **limits** | Maximum usable resources | Not used for scheduling | cgroup hard cap (OOMKilled if exceeded) |

## QoS Classes

The kubelet assigns QoS class based on request/limit configuration:

| QoS Class | Condition | Eviction Priority |
|-----------|-----------|-------------------|
| **Guaranteed** | requests == limits for all containers | Last to be evicted |
| **Burstable** | At least one container has requests < limits | Middle priority |
| **BestEffort** | No requests or limits specified | First to be evicted |

## Eviction Thresholds

kubelet monitors node resources and evicts Pods when thresholds are crossed:

| Threshold Type | Default | Behavior |
|----------------|---------|----------|
| **Hard** (`--eviction-hard`) | memory.available < 100Mi | Immediate eviction, no grace period |
| **Soft** (`--eviction-soft`) | memory.available < 200Mi | Graceful eviction with configurable grace period |

Eviction follows QoS priority: BestEffort first, then Burstable (proportional to overuse), and Guaranteed only as last resort.

## ResourceQuota and LimitRange

- **ResourceQuota**: Namespace-level aggregate limits (total CPU, memory, PVC count, Pod count)
- **LimitRange**: Per-container defaults and constraints (default requests/limits, min/max)

## Best Practices

- Always set both requests and limits for CPU and memory
- Use VPA to right-size resource requests based on actual usage
- Memory limits should account for JVM heap + off-heap (Metaspace, direct buffers, thread stacks)
- Set CPU limits carefully -- too low causes throttling; consider removing CPU limits for latency-sensitive workloads

## Related

- [[skills/learn-lecturer-persona.md|learn-lecturer-persona]] — K8S 讲师角色设定与场景规范
- [[skills/node-drain-and-maintenance.md|node-drain-and-maintenance]] — 节点驱逐与维护
- [[concepts/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[entities/kubelet.md|kubelet]] — kubelet
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]
- [[concepts/scheduling-algorithm.md|Scheduling Algorithm]]
- [[pod-lifecycle|Pod Lifecycle]]
- [[entities/kubelet.md|kubelet]]

- [[domain-02-workloads-applications/23-resource-management.md|23-resource-management]]