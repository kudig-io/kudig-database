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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-scheduler

> kube-scheduler 是 Kubernetes 控制平面的核心组件，负责监听未调度的 Pod 并将其分配到最合适的节点。它是唯一写入 `Pod.spec.nodeName` 的组件。

## 基本信息

| 属性 | 值 |
|------|------|
| 类型 | 控制平面组件 |
| 运行方式 | 静态 Pod / Deployment |
| 端口 | 10259 (HTTPS), 10251 (metrics) |
| HA | Leader Election (Lease) |
| 配置 | KubeSchedulerConfiguration |

## 调度框架 (Scheduling Framework)

### 扩展点

```
Pod 入队
    │
    ▼
┌─────────────────────────────────────────────────┐
│  Scheduling Cycle (串行)                        │
│                                                  │
│  PreFilter → Filter → PostFilter               │
│      │          │          │                    │
│      │          │          └─ 抢占 (Preemption)  │
│      │          └─ 节点可行性过滤            │
│      └─ 快速预检查                        │
│                                                  │
│  PreScore → Score → NormalizeScore             │
│      │          │          │                    │
│      │          │          └─ 分数归一化        │
│      │          └─ 节点打分排名            │
│      └─ 打分准备                        │
│                                                  │
│  Reserve → Permit → PreBind → Bind → PostBind │
│      │         │         │        │        │    │
│      │         │         │        │        └─ 清理│
│      │         │         │        └─ 分配节点  │
│      │         │         └─ 预绑定操作    │
│      │         └─ 绑定审批            │
│      └─ 资源预留                    │
└─────────────────────────────────────────────────┘
```

### 扩展点详解

| 阶段 | 扩展点 | 作用 | 示例插件 |
|------|--------|------|----------|
| 队列 | QueueSort | Pod 排序 | PrioritySort |
| 预过滤 | PreFilter | 快速预检查 | NodeResourcesFit |
| 过滤 | Filter | 节点可行性 | NodeAffinity, TaintToleration |
| 后过滤 | PostFilter | 抢占 | DefaultPreemption |
| 预打分 | PreScore | 打分准备 | InterPodAffinity |
| 打分 | Score | 节点排名 | NodeResourcesBalancedAllocation |
| 预留 | Reserve | 资源预留 | VolumeBinding |
| 许可 | Permit | 绑定审批 | - |
| 预绑定 | PreBind | 预绑定操作 | VolumeBinding |
| 绑定 | Bind | 节点分配 | DefaultBinder |
| 后绑定 | PostBind | 清理通知 | - |

## 默认插件详解

| 插件 | 阶段 | 功能 |
|------|------|------|
| NodeResourcesFit | Filter/Score | 资源请求匹配 |
| NodeAffinity | Filter/Score | 节点亲和性 |
| TaintToleration | Filter/Score | 污点容忍 |
| InterPodAffinity | Filter/Score | Pod 间亲和/反亲和 |
| PodTopologySpread | Filter/Score | 拓扑分布约束 |
| VolumeBinding | Filter/Reserve/PreBind | 卷绑定 |
| ImageLocality | Score | 镜像本地性 |
| NodeResourcesBalancedAllocation | Score | 资源均衡 |
| PrioritySort | QueueSort | 优先级排序 |
| DefaultPreemption | PostFilter | 默认抢占 |
| DefaultBinder | Bind | 默认绑定 |

## 调度配置

### KubeSchedulerConfiguration

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
leaderElection:
  leaderElect: true
  leaseDuration: 15s
  renewDeadline: 10s
  retryPeriod: 2s
profiles:
- schedulerName: default-scheduler
  plugins:
    score:
      enabled:
      - name: NodeResourcesBalancedAllocation
        weight: 1
      - name: ImageLocality
        weight: 1
      disabled:
      - name: NodeResourcesLeastAllocated
  pluginConfig:
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        type: MostAllocated
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
```

### 调度策略示例

```yaml
# Pod 调度约束
spec:
  schedulerName: default-scheduler
  nodeSelector:
    disktype: ssd
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: kubernetes.io/os
            operator: In
            values: [linux]
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: web
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
      matchLabels:
        app: web
```

## 运维操作

### 常用命令

```bash
# 🟢 查看调度器状态
kubectl get componentstatuses
kubectl get pods -n kube-system -l component=kube-scheduler

# 🟢 查看未调度 Pod
kubectl get pods --field-selector=status.phase=Pending -A

# 🟢 查看调度事件
kubectl describe pod <pod-name> | grep -A 10 Events
kubectl get events --field-selector reason=FailedScheduling

# 🟢 查看节点资源
kubectl describe nodes | grep -A 5 "Allocated resources"
kubectl top nodes

# 🟢 查看调度器指标
curl -k https://localhost:10259/metrics

# 🟢 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler --tail=100
```

## 故障排查

### 常见调度失败原因

| 原因 | 事件消息 | 解决方案 |
|------|----------|----------|
| 资源不足 | Insufficient cpu/memory | 扩容节点/减少 requests |
| 污点不容忍 | node(s) had taint | 添加 toleration |
| 亲和性不满足 | didn't match node selector | 检查 nodeSelector/affinity |
| PVC 未绑定 | pod has unbound PVC | 检查 StorageClass/PV |
| 拓扑约束 | topology spread constraint | 调整 maxSkew/whenUnsatisfiable |
| 抢占 | preempted by higher priority | 检查 PriorityClass |

### 排查流程

```
1. 确认 Pod Pending
   kubectl get pod <name> -o wide
       │
2. 查看事件
   kubectl describe pod <name> | grep -A 20 Events
       │
3. 检查节点资源
   kubectl describe nodes | grep -A 10 "Allocated"
       │
4. 检查污点/亲和性
   kubectl get nodes -o json | jq '.items[].spec.taints'
       │
5. 检查 PVC
   kubectl get pvc -n <ns>
```

## 调度性能优化

| 优化项 | 方法 | 效果 |
|--------|------|------|
| 并行调度 | percentageOfNodesToScore | 大集群加速 |
| 减少插件 | 禁用不需要的插件 | 减少延迟 |
| 优先级队列 | PriorityClass | 重要 Pod 优先 |
| 抢占 | Preemption | 高优先级抢占低优先级 |
| 多 Profile | schedulerName | 不同工作负载不同策略 |

```yaml
# 大集群优化: 只评估 30% 节点
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
percentageOfNodesToScore: 30
```

## HA 架构

```
┌─────────────────────────────────────┐
│  kube-scheduler-1 (Leader)     │  ← 实际调度
│  kube-scheduler-2 (Standby)    │  ← 待命
│  kube-scheduler-3 (Standby)    │  ← 待命
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│  Lease (kube-system/kube-scheduler) │
│  leaseDuration: 15s                 │
│  renewDeadline: 10s                 │
│  retryPeriod: 2s                    │
└─────────────────────────────────────┘
```

## 生产案例

### 案例1：Pod 长时间 Pending

**症状：** Pod Pending 超过 5 分钟

**根因：** 所有节点 CPU requests 已满

**解决：** 扩容节点 / 调整 requests / 使用 Cluster Autoscaler

### 案例2：调度延迟高

**症状：** 批量创建 Pod 时调度延迟 > 10s

**根因：** 5000 节点集群，每次调度遍历所有节点

**解决：** 设置 `percentageOfNodesToScore: 30`

## 检查清单

- [ ] 理解调度框架 11 个扩展点
- [ ] 掌握默认插件功能
- [ ] 能配置 KubeSchedulerConfiguration
- [ ] 掌握调度失败排查流程
- [ ] 理解抢占机制
- [ ] 能配置拓扑分布约束
- [ ] 了解 HA 和 Leader Election
- [ ] 掌握大集群调度优化

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/resource-management.md|Resource Management]]
- [[概念/scheduling-algorithm.md|Scheduling Algorithm]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[概念/high-availability-patterns.md|High Availability Patterns]]

<!-- risk-assessed -->
