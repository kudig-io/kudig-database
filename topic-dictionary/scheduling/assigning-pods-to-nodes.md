# Assigning Pods to Nodes

## 概述

Kubernetes 提供了多种方式将 Pod 约束到特定节点运行，或让 Pod 优先在某些节点上运行。推荐的方法都使用标签选择器（label selectors）来促进选择。虽然通常不需要设置此类约束（调度器会自动进行合理放置），但在某些情况下，用户可能需要控制 Pod 部署到哪个节点。

## 核心概念/原理

Kubernetes 提供了以下几种节点选择机制：

1. **nodeSelector**：最简单的节点选择约束，通过在 Pod 规范中添加 `nodeSelector` 字段，指定目标节点必须具有的标签。
2. **亲和性与反亲和性（Affinity and Anti-affinity）**：比 `nodeSelector` 更富有表现力的约束语言。
   - **Node affinity（节点亲和性）**：基于节点标签约束 Pod 可以调度到哪些节点，功能类似 `nodeSelector` 但更灵活，支持软规则（preferred）。
   - **Inter-pod affinity/anti-affinity（Pod 间亲和性/反亲和性）**：基于其他 Pod 的标签来约束 Pod 的放置，支持将相关 Pod 放置在同一个拓扑域或分散放置。
3. **nodeName**：更直接的节点选择方式，如果 `nodeName` 字段不为空，调度器会忽略该 Pod，直接由指定节点上的 kubelet 尝试放置。这种方式会绕过调度器。
4. **Pod topology spread constraints（Pod 拓扑分布约束）**：控制 Pod 在集群中的分布方式，如跨区域、跨节点等。

## 关键机制或特性

### Node Affinity 类型

- `requiredDuringSchedulingIgnoredDuringExecution`：硬约束，除非规则满足，否则调度器不会调度该 Pod。
- `preferredDuringSchedulingIgnoredDuringExecution`：软约束，调度器尝试找到满足规则的节点，如果没有可用节点，仍然调度该 Pod。

### Pod 间亲和性/反亲和性类型

同样支持 `requiredDuringSchedulingIgnoredDuringExecution` 和 `preferredDuringSchedulingIgnoredDuringExecution`。

- `podAffinity`：吸引 Pod，将 Pod 放置在已存在满足条件 Pod 的拓扑域中。
- `podAntiAffinity`：排斥 Pod，避免将 Pod 放置在已存在满足条件 Pod 的拓扑域中。

### 特殊字段

- **namespaceSelector**（v1.24+ stable）：通过标签选择命名空间。
- **matchLabelKeys**（v1.33+ stable）：指定 Pod 标签键，用于在计算 Pod（反）亲和性时与 incoming Pod 的标签匹配。
- **mismatchLabelKeys**（v1.33+ stable）：指定不应与 incoming Pod 标签匹配的键。
- **nominatedNodeName**（v1.35+ beta）：允许外部组件为 pending Pod 提名节点。

### 操作符

- `In`、`NotIn`、`Exists`、`DoesNotExist`：可用于 nodeAffinity 和 podAffinity。
- `Gt`、`Lt`：仅可用于 nodeAffinity。

## 使用场景

- 将 Pod 调度到带有 SSD 的节点上。
- 将通信频繁的两个不同服务的 Pod 放置到同一个可用区中。
- 节点隔离/限制：确保特定 Pod 只运行在具有特定隔离、安全或合规属性的节点上。
- 高可用部署：使用 Pod 反亲和性将副本分散到不同节点或可用区。

## 最佳实践/注意事项

- 如果使用标签进行节点隔离，应选择 kubelet 无法修改的标签键（如 `node-restriction.kubernetes.io/` 前缀），以防止受损节点自行设置这些标签。
- 不建议在超过数百个节点的大型集群中使用 Pod 间亲和性/反亲和性，因为这会显著降低调度速度。
- Pod 反亲和性要求集群中所有节点都一致地标记了 `topologyKey`，否则可能导致意外行为。
- 使用 `nodeName` 会绕过调度器，可能导致节点超订（oversubscribed），建议使用节点亲和性或 `nodeSelector`。
- 如果同时指定了 `nodeSelector` 和 `nodeAffinity`，两者都必须满足。
- `nodeSelectorTerms` 中的多个 terms 是 OR 关系；单个 `matchExpressions` 中的多个表达式是 AND 关系。

## 参考链接

- [Kubernetes 官方文档 - Assigning Pods to Nodes](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)
