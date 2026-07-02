---
title: Autoscaling Strategies
description: Autoscaling Strategies — Kubernetes 生产运维知识库
summary: Autoscaling Strategies — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- autoscaling
- hpa
- vpa
- cluster-autoscaler
- karpenter
- prometheus
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Autoscaling Strategies 是什么
- 如何 Autoscaling Strategies
trigger_keywords:
- Autoscaling
- Strategies
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Autoscaling Strategies

## Horizontal Pod Autoscaler (HPA)

HPA scales the number of Pod replicas based on observed metrics:

| Metric Type | Source | Example |
|-------------|--------|---------|
| **CPU/Memory** | metrics-server | Scale when avg CPU > 70% |
| **Custom metrics** | Prometheus adapter | Scale on requests per second |
| **External metrics** | Cloud provider | Scale on queue depth |

**Key behaviors**:
- Polling interval: ~15 seconds (default)
- Stabilization window: Prevents flapping (scale-up: 0s, scale-down: 5min default)
- Works with Deployment, [[StatefulSet|StatefulSet]], [[ReplicaSet|ReplicaSet]]

## Vertical Pod Autoscaler (VPA)

VPA adjusts resource requests/limits based on actual usage:

| Mode | Behavior | Production Safe |
|------|----------|-----------------|
| **Off** | Only recommends | Yes |
| **Initial** | Sets on Pod creation | Yes |
| **Auto** | Updates existing [[Pods|Pods]] (recreates) | Yes, with caution |
| **Recreate** | Same as Auto | Yes, with caution |

**Warning**: VPA and HPA on the same resource (CPU/memory) will conflict -- use VPA for right-sizing and HPA for replica scaling.

## Cluster Autoscaler / Karpenter

- **Cluster Autoscaler**: Traditional node scaler; adds/removes nodes based on unschedulable Pods
- **Karpenter**: Next-generation, faster node provisioning; supports spot instances, flexible instance selection, and TTL-based cleanup

## Scaling Strategy Recommendations

- Use **HPA** for demand-driven replica scaling (stateless services)
- Use **VPA** for right-sizing resource requests (all workloads)
- Use **Cluster Autoscaler** or **Karpenter** for node pool scaling
- Combine HPA + VPA (on different metrics) + Cluster Autoscaler for full automation

## Related

- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[concepts/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[concepts/resource-management.md|Resource Management]]
- [[skills/configure-health-probes.md|Configure Health Probes]]
- [[concepts/scheduling-algorithm.md|Scheduling Algorithm]]


<!-- risk-assessed -->
