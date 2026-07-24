---
title: Scheduling Algorithm
description: '- [[实体/scheduling-terms.md|scheduling-terms]] — K8s 调度术语参考'
summary: '- [[实体/scheduling-terms.md|scheduling-terms]] — K8s 调度术语参考'
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
tier: core
created: '2026-05-23'
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
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

## 源码实现分析

### 调度框架扩展点

```
Pod 入队
    │
    ├── QueueSort: 优先级排序
    ├── PreFilter: 快速预检查
    ├── Filter: 过滤不可用节点
    │     ├── NodeResourcesFit: CPU/内存/GPU
    │     ├── NodeAffinity: 节点亲和性
    │     ├── TaintToleration: 污点容忍
    │     └── PodTopologySpread: 拓扑约束
    ├── PostFilter: 抢占逻辑
    ├── PreScore: 预评分
    ├── Score: 节点打分
    │     ├── LeastAllocated: 资源均衡
    │     ├── ImageLocality: 镜像本地性
    │     └── InterPodAffinity: Pod 亲和性
    ├── NormalizeScore: 归一化 0-100
    ├── Reserve: 预留资源
    ├── Permit: 批准/拒绝/等待
    ├── PreBind: 卷绑定
    ├── Bind: 写入 nodeName
    └── PostBind: 清理
```

### 调度决策示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: web
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: zone
            operator: In
            values: [us-east-1a, us-east-1b]
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels: {app: web}
          topologyKey: kubernetes.io/hostname
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
  topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
    labelSelector:
      matchLabels: {app: web}
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| requests 就是实际使用量 | requests 是调度依据，不是实际消耗 |
| 调度后不能迁移 | Pod 绑定节点后不会自动迁移 |
| Priority 高就能抢占 | 还需满足 PDB 和抢占策略 |
| nodeSelector 等于 affinity | affinity 更灵活，支持软约束 |
| 调度器是单线程 | 支持并行调度 (percentageOfNodesToScore) |

## 面试要点

1. **Kubernetes 调度器的 Filter 和 Score 分别做什么？**
   - Filter: 排除不满足条件的节点（硬性约束）
   - Score: 对剩余节点打分排序（软性偏好）

2. **如何实现 Pod 的高可用分布？**
   - topologySpreadConstraints 跨可用区分布
   - podAntiAffinity 跨节点分布
   - maxSkew 控制分布偏差

3. **抢占 (Preemption) 是如何工作的？**
   - 无节点满足时触发
   - 驱逐低优先级 Pod
   - 尊重 PodDisruptionBudget
   - 被抢占 Pod 获得优雅终止时间

## Related

- [[实体/scheduling-terms.md|scheduling-terms]] — K8s 调度术语参考
- [[实体/k8s-workload-management.md|k8s-workload-management]] — 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
- [[实体/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[score]] — Score
- [[实体/kube-scheduler.md|kube-scheduler]]
- PriorityClass
- [[pod-lifecycle|Pod Lifecycle]]
- [[概念/resource-management.md|Resource Management]]


<!-- risk-assessed -->
