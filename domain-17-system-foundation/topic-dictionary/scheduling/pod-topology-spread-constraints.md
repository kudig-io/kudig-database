---
title: Pod Topology Spread Constraints
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Topology Spread Constraints 是什么
- 如何 Pod Topology Spread Constraints
trigger_keywords:
- Pod
- Topology
- Spread
- Constraints
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Pod Topology Spread Constraints

## 概述

Pod 拓扑分布约束（Pod Topology Spread Constraints）用于控制 Pod 在集群中的分布方式，使其跨故障域（如区域、可用区、节点）或其他用户定义的拓扑域均匀分布。这有助于实现高可用性和高效的资源利用。

## 核心概念/原理

该功能通过在 Pod API 的 `spec.topologySpreadConstraints` 字段中定义约束来实现。调度器会根据这些约束，将 incoming Pod 放置在相对于现有 Pod 的合适位置。

### 字段定义

- **maxSkew**：Pod 分布不均匀的最大允许程度，必须大于零。
  - 当 `whenUnsatisfiable: DoNotSchedule` 时，maxSkew 定义目标拓扑域中匹配 Pod 数量与全局最小值之间的最大允许差异。
  - 当 `whenUnsatisfiable: ScheduleAnyway` 时，调度器会优先选择有助于减少 skew 的拓扑域。
- **minDomains**（可选）：合格域的最小数量。当合格域数量少于 `minDomains` 时，全局最小值视为 0。仅能与 `DoNotSchedule` 一起使用，默认为 1 的行为。
- **topologyKey**：节点标签的键。具有相同键值对的节点被视为同一拓扑域。
- **whenUnsatisfiable**：
  - `DoNotSchedule`（默认）：如果不满足约束，不调度该 Pod。
  - `ScheduleAnyway`：即使不满足约束也调度，但优先选择能减少 skew 的节点。
- **labelSelector**：用于查找匹配的 Pod，以确定相应拓扑域中的 Pod 数量。
- **matchLabelKeys**（v1.27+ beta）：Pod 标签键列表，用于选择计算分布 skew 的 Pod 组。
- **nodeAffinityPolicy**（v1.26+ beta, v1.33 GA）：
  - `Honor`：只将匹配 nodeAffinity/nodeSelector 的节点纳入计算。
  - `Ignore`：忽略 nodeAffinity/nodeSelector，所有节点都纳入计算。
- **nodeTaintsPolicy**（v1.26+ beta, v1.33 GA）：
  - `Honor`：无污点的节点以及 Pod 有容忍的污点节点纳入计算。
  - `Ignore`：忽略节点污点，所有节点都纳入计算。

## 关键机制或特性

- **多约束组合**：当 Pod 定义多个 `topologySpreadConstraint` 时，约束之间使用逻辑 AND 运算。
- **同一 topologyKey + whenUnsatisfiable 只能有一个约束**。
- **隐式约定**：
  - 只考虑与 incoming Pod 同一命名空间的 Pod 作为匹配候选。
  - 调度器只考虑同时具有所有 `topologyKey` 的节点；缺少任何 `topologyKey` 的节点会被跳过。
- **集群级默认约束**：可以通过 `PodTopologySpread` 插件参数设置集群级默认约束。内置默认约束为：
  - `maxSkew: 3`, `topologyKey: [[Kubernetes|kubernetes]].io/hostname`, `whenUnsatisfiable: ScheduleAnyway`
  - `maxSkew: 5`, `topologyKey: topology.kubernetes.io/zone`, `whenUnsatisfiable: ScheduleAnyway`
- **与 podAffinity/podAntiAffinity 的区别**：拓扑分布约束提供对 Pod 在不同拓扑域中分布的更精细控制，既能实现高可用也能实现成本节约。
- **已知限制**：
  - Pod 被移除后（如缩容），不保证约束仍然满足。
  - 调度器不了解集群所有区域，只能从现有节点确定拓扑域，这可能在自动伸缩集群中导致问题。
  - Pod 标签不匹配自身的 `labelSelector` 会产生"幽灵 Pod"，导致分布约束以非预期方式工作。

## 使用场景

- 自动伸缩的副本集希望在节点或可用区之间均匀分布，以避免单点问题。
- 跨数据中心的客户端需要低延迟访问，希望副本均匀分布在各个基础设施区域。
- 滚动更新时平滑扩展副本，保持集群负载均衡。

## 最佳实践/注意事项

- 应该在同一组中的所有 Pod 上设置相同的拓扑分布约束。
- 确保拓扑域中的所有节点都一致地标记了拓扑标签。
- 如果节点不期望同时具有 `kubernetes.io/hostname` 和 `topology.kubernetes.io/zone` 标签，应该定义自己的约束而不是依赖 Kubernetes 默认值。
- 确保 Pod 的标签与其 `topologySpreadConstraints` 中的 `labelSelector` 匹配。
- 缩容后分布可能失衡，可以使用 Descheduler 等工具重新平衡 Pod 分布。

## 生产 YAML 示例

### 跨可用区 + 跨节点双层拓扑分布

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-frontend
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app: web-frontend
  template:
    metadata:
      labels:
        app: web-frontend
    spec:
      topologySpreadConstraints:
        # 跨可用区均匀分布（硬约束）
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: web-frontend
          nodeAffinityPolicy: Honor        # 只考虑 affinity 匹配的节点
          nodeTaintsPolicy: Honor          # 只考虑无污点/有容忍的节点
        # 跨节点均匀分布（软约束）
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: ScheduleAnyway
          labelSelector:
            matchLabels:
              app: web-frontend
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: topology.kubernetes.io/zone
                    operator: In
                    values: ["us-east-1a", "us-east-1b", "us-east-1c"]
      containers:
        - name: frontend
          image: registry.example.com/frontend:v4.2
          resources:
            requests:
              cpu: "250m"
              memory: 256Mi
            limits:
              cpu: "500m"
              memory: 512Mi
```

### 使用 matchLabelKeys 实现滚动更新平滑分布

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
  namespace: production
spec:
  replicas: 4
  selector:
    matchLabels:
      app: api-server
  template:
    metadata:
      labels:
        app: api-server
    spec:
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-server
          matchLabelKeys:
            - pod-template-hash            # 按 revision 分组计算 skew
      containers:
        - name: api
          image: registry.example.com/api:v3.0
          resources:
            requests:
              cpu: "500m"
              memory: 512Mi
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod Pending，不满足拓扑约束 | maxSkew=1 + DoNotSchedule 且节点/区域数量不均 | 适当放宽 maxSkew 或改用 ScheduleAnyway |
| 缩容后 Pod 分布不均 | 拓扑约束只在调度时生效，不自动重平衡 | 使用 Descheduler 重新平衡 Pod 分布 |
| 自动扩容后 Pod 仍集中在旧节点 | 新节点未标记拓扑标签 | 确认新节点有 `topology.kubernetes.io/zone` 和 `kubernetes.io/hostname` 标签 |
| "幽灵 Pod" 导致 skew 计算错误 | Pod 标签不匹配自身的 labelSelector | 确保 Pod 标签包含 labelSelector 中的所有键 |
| 滚动更新时新旧版本分布不均 | 未使用 matchLabelKeys | 添加 `matchLabelKeys: [pod-template-hash]` |

## 生产检查清单

- [ ] 为高可用服务配置跨区域（zone）+ 跨节点（hostname）双层拓扑约束
- [ ] 确保所有节点一致标记 `topology.kubernetes.io/zone` 和 `kubernetes.io/hostname`
- [ ] 确认 Pod 标签与 `topologySpreadConstraints.labelSelector` 匹配
- [ ] 滚动更新场景使用 `matchLabelKeys: [pod-template-hash]`
- [ ] 设置 `nodeAffinityPolicy: Honor` 和 `nodeTaintsPolicy: Honor` 排除不相关节点
- [ ] 使用 Descheduler `RemovePodsViolatingTopologySpreadConstraint` 策略自动重平衡
- [ ] 与 Karpenter / Cluster Autoscaler 配合，确保多区域有足够节点

## 命令快速参考

```bash
# 查看 Pod 分布在各节点/区域的情况
kubectl get pods -l app=web-frontend -o wide

# 按区域统计 Pod 数量
kubectl get pods -l app=web-frontend -o jsonpath='{range .items[*]}{.spec.nodeName}{"\n"}{end}' | \
  xargs -I{} kubectl get node {} -o jsonpath='{.metadata.labels.topology\.kubernetes\.io/zone}{"\n"}' | sort | uniq -c

# 查看 Pod 的拓扑约束配置
kubectl get pod <pod-name> -o jsonpath='{.spec.topologySpreadConstraints}' | jq .

# 查看集群默认拓扑约束
kubectl get cm -n kube-system kube-scheduler-config -o yaml | grep -A 10 PodTopologySpread

# 查看节点的拓扑标签
kubectl get nodes -o custom-columns='NAME:.metadata.name,ZONE:.metadata.labels.topology\.kubernetes\.io/zone'
```

## 交叉引用

- [将 Pod 分配给节点](./assigning-pods-to-nodes.md) — podAntiAffinity 与拓扑分布约束的对比
- [调度器性能调优](./scheduler-performance-tuning.md) — 拓扑分布约束对调度性能的影响
- Karpenter 自动扩缩容](./karpenter-autoscaling.md) — Karpenter 感知拓扑分布约束进行节点选型
- [调度框架](./scheduling-framework.md) — PodTopologySpread 插件的扩展点

## 参考链接

- [Kubernetes 官方文档 - Pod Topology Spread Constraints](https://kubernetes.io/docs/concepts/scheduling-eviction/topology-spread-constraints/)

## Related

- index/etcd-index|[[etcd|etcd]]cd 知识图谱索引|etcd 知识图谱索引]]]]
- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]
