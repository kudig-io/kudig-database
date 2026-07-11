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
- **调度后标签变化不迁移**："IgnoredDuringExecution"意味着节点 label 变化不会驱逐已调度 Pod，需 descheduler。
- **跨 NS 反亲和失效**：未声明 `namespaces` 时只看本 NS，跨 NS 重复副本不会被避开。
- **与 node-taint 冲突**：Taint 过滤掉节点后，反亲和可能把副本挤到剩余节点，副本集中。

## 参见

- [[kubernetes]] — k8s 领域核心页面
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]]
- [[概念/node-taint.md|节点污点]] — 基于节点的反向约束
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
