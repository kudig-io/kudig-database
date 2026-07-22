---
title: Resource Management (Requests, Limits, QoS)
description: Resource Management (Requests, Limits, QoS) — Kubernetes 生产运维知识库
summary: Resource Management (Requests, Limits, QoS) — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Resource Management (Requests, Limits, QoS)

## Requests vs Limits

| Field | Purpose | Scheduling | Runtime |
|-------|---------|------------|---------|
| **requests** | Minimum guaranteed resources | Used by scheduler to find fitting nodes | cgroup guaranteed share |
| **limits** | Maximum usable resources | Not used for scheduling | cgroup hard cap (OOMKilled if exceeded) |

## QoS Classes

The [[kubelet|kubelet]] assigns QoS class based on request/limit configuration:

| QoS Class | Condition | Eviction Priority |
|-----------|-----------|-------------------|
| **Guaranteed** | requests == limits for all containers | Last to be evicted |
| **Burstable** | At least one container has requests < limits | Middle priority |
| **BestEffort** | No requests or limits specified | First to be evicted |

## Eviction Thresholds

kubelet monitors node resources and evicts [[Pods|Pods]] when thresholds are crossed:

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

## 实践示例

### 资源配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  containers:
  - name: app
    image: myapp:1.0
    resources:
      requests:          # 调度依据 + 最低保证
        cpu: 250m
        memory: 256Mi
      limits:            # 硬上限
        cpu: "1"
        memory: 1Gi
---
# QoS 类别判断:
# requests != limits → Burstable
# requests == limits → Guaranteed
# 无 requests/limits → BestEffort
```

### ResourceQuota + LimitRange

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: team-a
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "50"
    persistentvolumeclaims: "10"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: team-a
spec:
  limits:
  - type: Container
    default:
      cpu: 500m
      memory: 512Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "4"
      memory: 8Gi
    min:
      cpu: 50m
      memory: 64Mi
```

## 源码实现分析

### kubelet 资源管理与 QoS 实现

```go
// k8s.io/kubernetes/pkg/kubelet/qos/policy.go
// QoS 分类逻辑
func GetPodQOS(pod *v1.Pod) v1.PodQOSClass {
    for _, container := range pod.Spec.Containers {
        requests := container.Resources.Requests
        limits := container.Resources.Limits
        
        // Guaranteed: 所有容器 requests == limits（CPU+Memory）
        if requests.Cpu().Cmp(*limits.Cpu()) == 0 &&
           requests.Memory().Cmp(*limits.Memory()) == 0 {
            continue
        }
        // Burstable: 至少一个容器设置了 requests 或 limits
        if len(requests) > 0 || len(limits) > 0 {
            return v1.PodQOSBurstable
        }
    }
    // BestEffort: 无任何资源设置
    return v1.PodQOSBestEffort
}

// k8s.io/kubernetes/pkg/kubelet/cm/cgroup_manager_linux.go
// cgroup v2 资源限制实现
func (m *cgroupManager) Set(resourceConfig *ResourceConfig) error {
    // CPU: cpu.max = "quota period" (e.g., "100000 100000" = 1 CPU)
    // Memory: memory.max = limit bytes
    // OOM: memory.oom.group = 1 (Guaranteed Pod)
    // CPU shares: cpu.weight (Burstable 按 requests 比例分配)
}
```

```
┌─────────────────────────────────────────────────────────┐
│     QoS 等级与资源保障                              │
├─────────────────────────────────────────────────────────┤
│  Guaranteed (requests == limits):                       │
│    └─ 最低 OOM 优先级，最后被驱逐                  │
│    └─ CPU: 独占 quota，不受其他 Pod 影响          │
│                                                         │
│  Burstable (部分设置):                                  │
│    └─ 中等 OOM 优先级                                  │
│    └─ CPU: 按 shares 比例分配空闲 CPU              │
│                                                         │
│  BestEffort (无设置):                                   │
│    └─ 最高 OOM 优先级，最先被驱逐                  │
│    └─ CPU: 最低 shares (2)，资源紧张时最先受影响  │
│                                                         │
│  驱逐顺序: BestEffort → Burstable → Guaranteed       │
└─────────────────────────────────────────────────────────┘
```

### 生产配置：资源管理最佳实践

```yaml
# 生产级资源配置
resources:
  requests:
    cpu: "500m"      # 基于 P95 实际使用量
    memory: "512Mi"  # 基于 P99 实际使用量
  limits:
    cpu: "2"         # 允许突发，但不超过 2 核
    memory: "1Gi"    # 硬限制，超过 OOM Kill
# 注意:
# - CPU 可压缩（throttle），Memory 不可压缩（OOM）
# - requests 影响调度，limits 影响运行时
# - 不设置 limits.cpu 可避免 CPU throttling
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| requests 是实际使用量 | requests 是调度依据和最低保证 |
| limits 越大越好 | 过大导致节点资源超卖，过小导致 OOM/Throttle |
| CPU limits 必须设置 | 延迟敏感服务可不设 CPU limits 避免节流 |
| 内存可以超卖 | 内存不可压缩，超出 limits 直接 OOMKilled |
| QoS 只是标签 | QoS 决定驱逐优先级，影响稳定性 |

## 面试要点

1. **requests 和 limits 的区别？**
   - requests: 调度依据 + cgroup 保证份额
   - limits: 硬上限，超出被节流 (CPU) 或 OOM (内存)

2. **三种 QoS 类别如何确定？**
   - Guaranteed: 所有容器 requests == limits
   - Burstable: 至少一个容器 requests < limits
   - BestEffort: 无任何 requests/limits

3. **节点压力时驱逐顺序？**
   - BestEffort 最先驱逐
   - Burstable 按超卖比例驱逐
   - Guaranteed 最后驱逐

4. **为什么 CPU 可以不设 limits？**
   - CPU 是可压缩资源，节流而非杀死
   - 节流导致延迟抨动
   - 延迟敏感服务建议仅设 requests

## Related

- [[技能/learn-lecturer-persona.md|learn-lecturer-persona]] — K8S 讲师角色设定与场景规范
- [[技能/node-drain-and-maintenance.md|node-drain-and-maintenance]] — 节点驱逐与维护
- [[概念/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[概念/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[实体/kubelet.md|kubelet]] — kubelet
- [[概念/autoscaling-strategies.md|Autoscaling Strategies]]
- [[概念/scheduling-algorithm.md|Scheduling Algorithm]]
- [[pod-lifecycle|Pod Lifecycle]]
- [[实体/kubelet.md|kubelet]]

- 23-resource-management

<!-- risk-assessed -->
