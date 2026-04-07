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

## 生产 YAML 示例

### 综合节点选择示例（nodeSelector + nodeAffinity + podAntiAffinity）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  template:
    metadata:
      labels:
        app: order-service
        version: v2
    spec:
      nodeSelector:
        disk-type: ssd                     # 硬约束：必须 SSD 节点
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: topology.kubernetes.io/zone
                    operator: In
                    values: ["us-east-1a", "us-east-1b", "us-east-1c"]
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 80
              preference:
                matchExpressions:
                  - key: node.kubernetes.io/instance-type
                    operator: In
                    values: ["m6i.xlarge", "m6i.2xlarge"]
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values: ["order-service"]
              topologyKey: kubernetes.io/hostname   # 同一节点不放两个副本
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values: ["order-service"]
                topologyKey: topology.kubernetes.io/zone  # 尽量跨可用区分布
      containers:
        - name: order
          image: registry.example.com/order:v2.3
          resources:
            requests:
              cpu: "500m"
              memory: 512Mi
            limits:
              cpu: "1"
              memory: 1Gi
```

### Pod 间亲和性（共置相关服务）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cache-sidecar
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: cache-sidecar
  template:
    metadata:
      labels:
        app: cache-sidecar
    spec:
      affinity:
        podAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values: ["order-service"]
              topologyKey: kubernetes.io/hostname  # 与 order-service 放在同一节点
      containers:
        - name: redis
          image: redis:7-alpine
          resources:
            requests:
              cpu: "100m"
              memory: 128Mi
```

## 节点选择机制对比

| 机制 | 表达力 | 推荐程度 | 说明 |
|------|--------|----------|------|
| `nodeSelector` | 简单 key=value | 推荐（简单场景） | 等价于 requiredDuringScheduling + In 操作 |
| `nodeAffinity` required | 丰富操作符 | 推荐（复杂场景） | 支持 In/NotIn/Exists/DoesNotExist/Gt/Lt |
| `nodeAffinity` preferred | 软约束 + 权重 | 推荐（优化场景） | 尽力满足，不阻塞调度 |
| `podAffinity` | 基于 Pod 标签 | 谨慎使用 | 大集群中性能开销大 |
| `podAntiAffinity` | 基于 Pod 标签 | 推荐（HA 场景） | 副本跨节点/跨区分散 |
| `nodeName` | 直接指定 | 不推荐 | 绕过调度器，可能导致超订 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod Pending，FailedScheduling | nodeSelector / affinity 无节点匹配 | `kubectl get nodes --show-labels` 检查节点标签是否满足 |
| Pod 被调度到非预期节点 | preferred 规则权重不够或无匹配节点 | 改用 required 规则；调整权重 |
| 使用 podAntiAffinity 后无法扩容 | 节点数少于副本数 | 增加节点或放宽 topologyKey 为 zone 级别 |
| 调度速度变慢 | podAffinity/podAntiAffinity 在大集群中性能差 | 减少使用 pod 间亲和性；改用 topologySpreadConstraints |
| nodeName Pod 导致节点超载 | 绕过了调度器资源检查 | 改用 nodeSelector / nodeAffinity |

## 生产检查清单

- [ ] 优先使用 `nodeSelector` 和 `nodeAffinity`，避免 `nodeName`
- [ ] 节点隔离标签使用 `node-restriction.kubernetes.io/` 前缀防止节点自行修改
- [ ] 高可用部署使用 `podAntiAffinity` + `topologyKey: kubernetes.io/hostname`
- [ ] 跨区域高可用追加 `topologyKey: topology.kubernetes.io/zone`
- [ ] 大集群（500+ 节点）谨慎使用 podAffinity/podAntiAffinity
- [ ] 确保所有节点一致标记 topologyKey 标签
- [ ] 同时设置 nodeSelector 和 nodeAffinity 时确认两者兼容

## 命令快速参考

```bash
# 查看节点标签
kubectl get nodes --show-labels

# 为节点添加标签
kubectl label nodes <node-name> disk-type=ssd

# 查看 Pod 被调度到哪个节点
kubectl get pods -o wide

# 查看 Pod 的亲和性配置
kubectl get pod <pod-name> -o jsonpath='{.spec.affinity}' | jq .

# 查看特定标签的节点
kubectl get nodes -l disk-type=ssd

# 模拟调度（dry-run）
kubectl run test --image=nginx --dry-run=server -o yaml --overrides='{"spec":{"nodeSelector":{"disk-type":"ssd"}}}'
```

## 交叉引用

- [污点与容忍度](./taints-and-tolerations.md) — 节点排斥机制，与亲和性互补
- [Pod 拓扑分布约束](./pod-topology-spread-constraints.md) — 更精细的跨拓扑域分布控制
- [Kubernetes 调度器](./kubernetes-scheduler.md) — 过滤和评分阶段如何处理亲和性
- [Karpenter 自动扩缩容](./karpenter-autoscaling.md) — Karpenter 如何感知 nodeSelector / affinity

## 参考链接

- [Kubernetes 官方文档 - Assigning Pods to Nodes](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)
