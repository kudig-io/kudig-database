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
  - `maxSkew: 3`, `topologyKey: kubernetes.io/hostname`, `whenUnsatisfiable: ScheduleAnyway`
  - `maxSkew: 5`, `topologyKey: topology.kubernetes.io/zone`, `whenUnsatisfiable: ScheduleAnyway`
- **与 podAffinity/podAntiAffinity 的区别**：拓扑分布约束提供对 Pod 在不同拓扑域中分布的更精细控制，既能实现高可用也能实现成本节约。
- **已知限制**：
  - Pod 被移除后（如缩容），不保证约束仍然满足。
  - 调度器不了解集群所有区域，只能从现有节点确定拓扑域，这可能在自动伸缩集群中导致问题。
  - Pod 标签不匹配自身的 `labelSelector` 会产生"幽灵 Pod"，导致分布约束以非预期方式工作。

## 使用场景

- 自动伸缩的副本集希望在节点或可用区之间均匀分布，以避免单点故障。
- 跨数据中心的客户端需要低延迟访问，希望副本均匀分布在各个基础设施区域。
- 滚动更新时平滑扩展副本，保持集群负载均衡。

## 最佳实践/注意事项

- 应该在同一组中的所有 Pod 上设置相同的拓扑分布约束。
- 确保拓扑域中的所有节点都一致地标记了拓扑标签。
- 如果节点不期望同时具有 `kubernetes.io/hostname` 和 `topology.kubernetes.io/zone` 标签，应该定义自己的约束而不是依赖 Kubernetes 默认值。
- 确保 Pod 的标签与其 `topologySpreadConstraints` 中的 `labelSelector` 匹配。
- 缩容后分布可能失衡，可以使用 Descheduler 等工具重新平衡 Pod 分布。

## 参考链接

- [Kubernetes 官方文档 - Pod Topology Spread Constraints](https://kubernetes.io/docs/concepts/scheduling-eviction/topology-spread-constraints/)
