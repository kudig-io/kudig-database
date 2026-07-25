---
title: 调度与拓扑分布生产模式
description: 生产级调度策略：拓扑分布约束、亲和性反亲和性、Spot/抢占式节点与 Descheduler 实践
summary: 生产级调度策略：拓扑分布约束、亲和性反亲和性、Spot/抢占式节点与 Descheduler 实践，含调度失败排障与高可用分布清单。
category: application-patterns
tags:
- scheduling
- topology-spread
- affinity
- spot
- descheduler
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 17min
intent_queries:
- K8s 调度生产模式是什么
- 如何用 topologySpreadConstraints 实现多可用区分布
trigger_keywords:
- 调度
- 拓扑分布
- 亲和性
- Spot
- Descheduler
prerequisites:
- kubectl-basics
- scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。

# 调度与拓扑分布生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

调度策略决定 Pod 分布在哪些节点上，直接影响可用性和成本。所有副本集中在同一节点或同一可用区，是该节点/AZ 故障时服务全灭的常见根因。本文涵盖 topologySpreadConstraints 多 AZ 分布、亲和性反亲和性、Spot 节点策略和 Descheduler 再平衡实践。

---

## 1. 拓扑分布约束 (topologySpreadConstraints)

### 1.1 为什么需要它

传统 `podAntiAffinity` 只能做"每节点最多 N 个"，无法表达"尽量均匀分布在各可用区"。`topologySpreadConstraints` (v1.19+ stable) 提供精确的跨域分布控制。

### 1.2 生产模板：跨 AZ 均匀分布

```yaml
spec:
  topologySpreadConstraints:
    - maxSkew: 1                 # 各域之间最大允许偏差 1 个 Pod
      topologyKey: topology.kubernetes.io/zone   # 按可用区分散
      whenUnsatisfiable: DoNotSchedule           # 不满足则不调度（严格）
      labelSelector:
        matchLabels:
          app: api-server
    - maxSkew: 1
      topologyKey: kubernetes.io/hostname        # 同时按节点分散
      whenUnsatisfiable: ScheduleAnyway          # 节点级尽量满足（宽松）
      labelSelector:
        matchLabels:
          app: api-server
```

### 1.3 maxSkew 与 whenUnsatisfiable 决策

| 参数 | 含义 | 生产建议 |
|---|---|---|
| `maxSkew: 1` | 域之间最多差 1 个 Pod | 核心服务严格模式 |
| `whenUnsatisfiable: DoNotSchedule` | 不满足约束 → Pending | 核心服务（宁可不调度也不集中） |
| `whenUnsatisfiable: ScheduleAnyway` | 尽量满足，不满足也调度 | 配合 `nodeSelector`/亲和性做尽力而为 |

> ⚠️ **生产陷阱**: 多 AZ 集群中，若某 AZ 节点不足，`DoNotSchedule` 会导致 Pod Pending。生产建议: AZ 级 `DoNotSchedule` + 节点级 `ScheduleAnyway` 组合，保证跨 AZ 强制分布但节点级尽力而为。

---

## 2. 亲和性与反亲和性

### 2.1 四种亲和性对比

| 类型 | 作用域 | 匹配方式 | 性能 | 典型用途 |
|---|---|---|---|---|
| `nodeAffinity (required)` | 节点标签 | 硬性约束 | 快 | 必须调度到 GPU 节点 |
| `nodeAffinity (preferred)` | 节点标签 | 软性偏好 | 快 | 倾向同 AZ 降低延迟 |
| `podAffinity` | 同域 Pod 标签 | 硬/软 | **慢（O(n²)）** | 缓存与 DB 同节点 |
| `podAntiAffinity` | 同域 Pod 标签 | 硬/软 | **慢（O(n²)）** | 副本分散到不同节点 |

> ⚠️ **性能警告**: `podAntiAffinity` 在大集群（> 100 节点）中显著拖慢调度。优先使用 `topologySpreadConstraints` 替代 `podAntiAffinity` 做分布控制，性能更好。

### 2.2 Spot/抢占式节点亲和性模板

```yaml
affinity:
  nodeAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100               # 强烈偏好 Spot
        preference:
          matchExpressions:
            - key: node.kubernetes.io/instance-type
              operator: In
              values: ["spot", "preemptible"]
  podAntiAffinity:                # Spot 上分散，单节点多副本降低驱逐影响
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 50
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: batch-worker
          topologyKey: kubernetes.io/hostname
tolerations:
  - key: "spot-instance"
    operator: "Exists"
```

---

## 3. Spot/抢占式节点策略

### 3.1 Spot 风险与缓解

Spot 节点可被云厂商随时回收（通常 30 秒告警），需要专门的容错设计：

| 风险 | 缓解策略 |
|---|---|
| Pod 被突然驱逐 | 配 `podDisruptionBudget` + 多副本跨节点分布 |
| 长任务中断 | Checkpoint 机制 + Job `backoffLimit` + 优雅终止 |
| 容量不足 | Spot + On-Demand 混合节点池（Karpenter/Cluster Autoscaler） |
| 通知窗口短 | 部署 `node-termination-handler` 提前 drain |

### 3.2 Spot 混合部署模板

```yaml
spec:
  replicas: 6
  template:
    spec:
      # 60% Spot + 40% On-Demand 的混合策略通过两个 Deployment 实现
      # 或使用 Karpenter 的 capacity type 偏好
      priorityClassName: spot-preferred
      containers:
        - name: worker
          # 应用需支持：信号处理 + 状态持久化 + 幂等重试
      tolerations:
        - key: karpenter.sh/capacity-type
          operator: Equal
          value: spot
          effect: NoSchedule
```

> 🟡 中风险。Spot 节点不适用于有状态服务（StatefulSet）。仅用于无状态、可重试、可中断的工作负载。

---

## 4. Descheduler 再平衡

随着节点增减、Pod 创建销毁，集群调度会逐渐"失衡"（某些节点过载，某些空闲）。Descheduler 周期性识别并驱逐不平衡的 Pod 触发重新调度。

### 4.1 生产策略配置

```yaml
apiVersion: "descheduler/v1alpha1"
kind: "DeschedulerPolicy"
strategies:
  RemoveDuplicates:              # 同一节点上相同 ReplicaSet 的重复 Pod
    enabled: true
  RemovePodsViolatingTopologySpreadConstraint:  # 违反拓扑分布的 Pod
    enabled: true
    params:
      includeSoftConstraints: false   # 仅处理 DoNotSchedule 类型
  RemovePodsViolatingInterPodAntiAffinity:  # 违反反亲和性的 Pod
    enabled: true
  LowNodeUtilization:            # 低利用率节点再平衡
    enabled: true
    params:
      nodeResourceUtilizationThresholds:
        targetThresholds:
          cpu: 50
          memory: 50
        thresholds:
          cpu: 20
          memory: 20
```

> ⚠️ **生产警告**: Descheduler 只负责**驱逐**不负责调度。驱逐前确认 PDB 已配置，否则可能中断服务。建议设 `dryRun: true` 先观察。

---

## 5. 生产检查清单

| # | 检查项 | 验证命令 | 合格标准 |
|---|---|---|---|
| 1 | 核心服务跨 AZ 分布 | `kubectl get pod -o wide \| awk '{print $7}' \| sort \| uniq -c` | 各 AZ Pod 数均匀(maxSkew ≤ 1) |
| 2 | topologySpreadConstraints 已配 | `kubectl get deploy -o yaml \| grep topologySpread` | 核心服务命中 |
| 3 | 无单点集中(同节点多副本) | `kubectl get pod -o wide --sort-by=.spec.nodeName` | 核心服务无 2+ 副本同节点 |
| 4 | Spot 工作负载有 PDB | `kubectl get pdb` | Spot Deployment 有 PDB |
| 5 | 优先级类别已配置 | `kubectl get priorityclass` | 关键服务有高优先级，Spot 低优先级 |
| 6 | 节点资源碎片可调度 | 检查 Pending Pod 原因 | 无因资源碎片 Pending 的核心 Pod |

---

## 6. 排障速查

| 症状 | 可能根因 | 诊断命令 | 修复 |
|---|---|---|---|
| Pod Pending | topologySpread DoNotSchedule 无满足域 | `kubectl describe pod` 看 Events | 扩展节点到更多 AZ 或改 ScheduleAnyway |
| Pod 集中某节点 | 缺少 topologySpread / nodeAffinity 偏好 | 检查调度约束配置 | 加 topologySpreadConstraints |
| 调度极慢 | podAntiAffinity 计算量大(大集群) | 检查 scheduler 日志 | 改用 topologySpreadConstraints |
| Spot Pod 频繁中断 | 无 PDB / 无优雅终止 | 检查 PDB + preStop | 加 PDB + node-termination-handler |
| 集群负载不均 | 无 Descheduler / 节点增减后未再平衡 | `kubectl describe node` 对比负载 | 部署 Descheduler (先 dryRun) |

---

## 7. 跨域协作

- **Pod 可用性与 PDB**: 见 [[04-应用模式/03-生产模式/pod-availability-lifecycle|Pod 可用性生产模式]]
- **资源 QoS 与 right-sizing**: 见 [[04-应用模式/03-生产模式/resource-qos-rightsizing|资源 QoS 与 Right-sizing]]
- **HPA/VPA 弹性伸缩**: 见 `工作负载/00-core-workloads/21-hpa-vpa-autoscaling.md`
- **Karpenter 节点供应**: 见 `集群基础/99-production-readiness-operations-guide.md`


<!-- risk-assessed -->
