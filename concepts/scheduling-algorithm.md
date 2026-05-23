---
title: Scheduling Algorithm
description: '- [[references/scheduling-terms.md|scheduling-terms]] — K8s 调度术语参考'
category: concepts
tags:
- k8s
- scheduling
- algorithm
- filter
- score
- preemption
- scheduler
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Scheduling Algorithm 是什么
- 如何 Scheduling Algorithm
trigger_keywords:
- Scheduling
- Algorithm
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Scheduling Algorithm

## Scheduling Cycle (Filter + Score)

The scheduler processes each unscheduled Pod through these phases:

1. **SchedulingQueue**: Pod enters priority-based queue
2. **PreFilter**: Fast pre-checks (resource totals, feature validation)
3. **Filter**: Eliminates nodes that cannot run the Pod (resources, affinity, taints, topology, volumes)
4. **PostFilter**: Preemption -- if no nodes pass, try evicting lower-priority [[Pods|Pods]]
5. **Score**: Rank remaining nodes (resource balance, image locality, topology spread, inter-pod affinity)
6. **NormalizeScore**: Normalize scores to 0-100 range
7. **Select**: Choose highest-scored node

## Binding Cycle

After node selection:
1. **Permit**: Approve, reject, or hold the binding
2. **PreBind**: Execute pre-bind actions (e.g., volume binding)
3. **Bind**: Update `Pod.spec.nodeName` in API Server
4. **PostBind**: Post-bind cleanup

## Key Plugins

| Plugin | Phase | Function |
|--------|-------|----------|
| NodeResourcesFit | Filter | Check CPU/memory/GPU fit |
| NodeAffinity | Filter | Match node labels |
| TaintToleration | Filter | Handle node taints |
| InterPodAffinity | Score | Spread or co-locate with other Pods |
| PodTopologySpread | Score | Distribute across failure domains |
| ImageLocality | Score | Prefer nodes with cached images |
| NodeResourcesBalancedAllocation | Score | Balance resource utilization |

## Preemption

When no node can satisfy a Pod, the scheduler may evict lower-priority Pods to make room. Preemption respects PodDisruptionBudgets and only evicts Pods with lower PriorityClass values.

## Related

- [[references/scheduling-terms.md|scheduling-terms]] — K8s 调度术语参考
- [[references/k8s-workload-management.md|k8s-workload-management]] — 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
- [[entities/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[score]] — Score
- [[entities/kube-scheduler.md|kube-scheduler]]
- PriorityClass
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/resource-management.md|Resource Management]]
