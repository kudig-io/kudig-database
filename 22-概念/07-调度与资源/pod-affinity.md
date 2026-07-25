---
title: Pod 亲和性
summary: Pod 亲和性：Kubernetes Pod 亲和性（Affinity）用于控制 Pod 在节点上的调度偏好。
category: concepts
tags:
- core-concept
- k8s
- scheduling
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# Pod 亲和性

## 概述

Pod 亲和性（Affinity）与反亲和性（Anti-Affinity）是 Kubernetes 调度器提供的、根据**其他 Pod 的位置**来决定当前 Pod 调度偏好的机制。它和 nodeSelector / nodeAffinity（基于节点本身的标签）互补：前者解决"我要和谁一起 / 不和谁一起"，后者解决"我要去什么样的节点"。典型用途：把缓存和应用调度到同一节点降低延迟（亲和）、把同一应用副本打散到不同节点/可用区提高可用性（反亲和）。

## 架构与工作原理

```
调度器（kube-scheduler）决策流程：
┌────────────────────────────────────────────────┐
│ 1. Filter（预选）：节点资源/污点/nodeAffinity     │
│ 2. Pod 亲和/反亲和：                             │
│    - topologyKey: 把节点按拓扑分组（zone/hostname）│
│    - 检查"该拓扑域内已有哪些 Pod"（用 labelSelector）│
│    - requiredDuringScheduling：硬约束，不满足过滤  │
│    - preferredDuringScheduling：软约束，打分加权   │
│ 3. Score（优选）：综合打分排序                   │
└────────────────────────────────────────────────┘

反亲和示例（打散）：
  topologyKey: kubernetes.io/hostname
  规则：同一 topologyKey 域内若已有 app=web 的 Pod，则排除该节点
  → 副本会分布到不同节点
```

**两类约束**：
- `requiredDuringSchedulingIgnoredDuringExecution`：**硬约束**。调度时必须满足，否则 Pod 一直 Pending。注意"IgnoredDuringExecution"——一旦调度后节点标签变了也不会驱逐。
- `preferredDuringSchedulingIgnoredDuringExecution`：**软约束**。带 weight（1-100），尽量满足，不满足也能调度。

**两类方向**：
- `podAffinity`：倾向于和匹配 Pod 在同一 topologyKey 域。
- `podAntiAffinity`：倾向于避开匹配 Pod 所在的 topologyKey 域。

**topologyKey 是核心**：决定"拓扑域"粒度。常用值：
- `kubernetes.io/hostname` → 节点级（同/不同节点）
- `topology.kubernetes.io/zone` → 可用区级（跨 AZ 打散）
- `kubernetes.io/os` → 几乎无意义（全局）

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `podAffinity` | 期望与某些 Pod 同域 |
| `podAntiAffinity` | 期望与某些 Pod 异域 |
| `labelSelector` | 匹配"参照 Pod"的标签 |
| `topologyKey` | 拓扑域的节点标签键 |
| `namespaces` | 跨命名空间匹配（默认同 NS） |
| `weight` | preferred 的权重（1-100） |
| `requiredDuringScheduling` | 硬约束，不满足 Pending |
| `preferredDuringScheduling` | 软约束，打分优化 |

## 配置示例

```yaml
---
# 反亲和：同一应用的副本打散到不同节点（硬约束）
apiVersion: apps/v1
kind: Deployment
metadata: {name: webapp, namespace: production}
spec:
  replicas: 3
  selector: {matchLabels: {app: webapp}}
  template:
    metadata: {labels: {app: webapp}}
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels: {app: webapp}
            topologyKey: kubernetes.io/hostname
      containers:
      - {name: webapp, image: webapp:v1}
---
# 软约束：尽量跨可用区 + 与 redis 同节点（亲和）
apiVersion: apps/v1
kind: Deployment
metadata: {name: cache-client, namespace: production}
spec:
  replicas: 3
  selector: {matchLabels: {app: cache-client}}
  template:
    metadata: {labels: {app: cache-client}}
    spec:
      affinity:
        podAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels: {app: redis}
              topologyKey: kubernetes.io/hostname
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 80
            podAffinityTerm:
              labelSelector:
                matchLabels: {app: cache-client}
              topologyKey: topology.kubernetes.io/zone
      containers:
      - {name: app, image: app:v1}
```

## 常用操作与命令

```bash
# 查看 Pod 实际落在哪些节点/可用区
kubectl get pods -n production -o wide \
  -l app=webapp --no-headers | awk '{print $1, $7}'

# 检查节点拓扑标签
kubectl get nodes --show-labels | grep -E 'zone|hostname'

# 给节点打可用区标签（云上一般自动打）
kubectl label node node-1 topology.kubernetes.io/zone=cn-east-1a

# 排查 Pending：describe 看 scheduler 的 FailedScheduling 事件
kubectl describe pod webapp-xxx | grep -A20 Events

# 用 descheduler 做再均衡（副本不均时）
#（需安装 descheduler，跨节点迁移 Pod）
```

## 最佳实践

1. **生产应用必加 podAntiAffinity**：至少把副本跨节点（`hostname`）打散，关键服务跨可用区（`zone`）。
2. **大规模集群用软约束**：硬约束在节点少时易 Pending，软约束 + 多 topologyKey 更鲁棒。
3. **topologyKey 节点必须都打**：若某些节点缺该标签，调度器会跳过它们，导致副本集中。
4. **跨命名空间注意 namespaces 字段**：默认只匹配同 NS Pod，跨 NS 协同需显式声明。
5. **结合 nodeSelector/nodeAffinity**：先用 nodeAffinity 限定机型（如 GPU 节点），再用 podAntiAffinity 打散。
6. **考虑 topologySpreadConstraints**：1.19+ 它比 podAntiAffinity 更精细控制均匀分布（maxSkew）。

## 常见陷阱

- **Pod 一直 Pending**：硬反亲和 + 节点不足；副本数 > 节点数必然 Pending，改用软约束。
- **topologyKey 标签不全**：部分节点没打 zone 标签，调度器把副本堆到有标签的少数节点。
- **大规模集群调度慢**：required 反亲和在 1000+ Pod 时 O(n²) 计算，用软约束或 topologySpread。
- **调度后标签变化不迁移**：“IgnoredDuringExecution”意味着节点 label 变化不会驱逐已调度 Pod，需 descheduler。
- **跨 NS 反亲和失效**：未声明 `namespaces` 时只看本 NS，跨 NS 重复副本不会被避开。
- **与 node-taint 冲突**：Taint 过滤掉节点后，反亲和可能把副本挤到剩余节点，副本集中。

## 源码实现分析

### 调度器亲和性插件

```go
// k8s.io/kubernetes/pkg/scheduler/framework/plugins/interpodaffinity/filtering.go
// InterPodAffinity 插件在 Filter 阶段评估 Pod 间亲和/反亲和
func (pl *InterPodAffinity) Filter(ctx context.Context, state *framework.CycleState,
    pod *v1.Pod, nodeInfo *framework.NodeInfo) *framework.Status {
    
    // 1. 检查 Pod 的亲和性规则
    affinity := pod.Spec.Affinity
    if affinity == nil || affinity.PodAntiAffinity == nil {
        return nil  // 无规则，通过
    }
    
    // 2. 遍历节点上所有现有 Pod
    for _, existingPod := range nodeInfo.Pods {
        for _, term := range affinity.PodAntiAffinity.RequiredDuringSchedulingIgnoredDuringExecution {
            // 3. 检查 topologyKey 是否匹配
            if topologyMatches(term.TopologyKey, pod, existingPod) {
                // 4. 检查 label selector 是否匹配
                if term.LabelSelector.Matches(existingPod.Labels) {
                    // 硬反亲和匹配：拒绝调度到此节点
                    return framework.NewStatus(framework.Unschedulable,
                        "pod anti-affinity conflict")
                }
            }
        }
    }
    return nil
}
```

### 调度流程中的亲和性评估

```
┌───────────────────────────────────────────────────────────┐
│        Pod 调度中亲和性评估流程                      │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  Pod 待调度                                              │
│    │                                                      │
│    ▼                                                      │
│  PreFilter: 收集亲和性信息                              │
│    │                                                      │
│    ▼                                                      │
│  Filter (每个候选节点):                                  │
│    ├─ NodeAffinity: 节点标签是否匹配                  │
│    ├─ PodAffinity: 目标 Pod 是否在同一拓扑域        │
│    ├─ PodAntiAffinity: 目标 Pod 是否不在同一拓扑域  │
│    └─ Taint/Toleration: 节点污点是否可容忍          │
│    │                                                      │
│    ▼                                                      │
│  Score (软约束打分):                                     │
│    ├─ preferred PodAffinity: +weight                    │
│    ├─ preferred PodAntiAffinity: -weight                │
│    └─ TopologySpread: 均匀分布加分                   │
│    │                                                      │
│    ▼                                                      │
│  选择最高分节点 → Bind                                  │
└───────────────────────────────────────────────────────────┘
```

### 生产配置示例（🟡 部署到集群）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 3
  template:
    spec:
      affinity:
        # 硬反亲和：副本必须跨可用区
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: web-app
            topologyKey: topology.kubernetes.io/zone
        # 软反亲和：尽量跨节点
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: web-app
              topologyKey: kubernetes.io/hostname
      # 更精细的均匀分布（1.19+）
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-app
```

## 面试要点

1. **Pod 亲和性与节点亲和性的区别？**
   - 节点亲和性：Pod 对节点标签的约束（替代 nodeSelector）
   - Pod 亲和性：Pod 对其他 Pod 位置的约束
   - 两者可组合使用

2. **硬约束 vs 软约束的选择？**
   - 硬（required）：必须满足，不满足则 Pending
   - 软（preferred）：尽量满足，不满足仍可调度
   - 生产建议：跨 AZ 用硬，跨节点用软

3. **topologySpreadConstraints vs podAntiAffinity？**
   - topologySpread：控制副本均匀分布（maxSkew）
   - podAntiAffinity：只表达“不要在一起”
   - 1.19+ 推荐用 topologySpread，更精细且性能更好

4. **大规模集群中亲和性的性能影响？**
   - required 反亲和是 O(n²)（遍历所有 Pod）
   - 1000+ Pod 时明显变慢
   - 解决：用 preferred 或 topologySpread 替代

## 参见

- [[kubernetes]] — k8s 领域核心页面
- [[22-概念/02-工作负载/pods.md|Pod]]
- [[22-概念/02-工作负载/deployments.md|Deployment]]
- [[22-概念/07-调度与资源/node-taint.md|节点污点]] — 基于节点的反向约束
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
