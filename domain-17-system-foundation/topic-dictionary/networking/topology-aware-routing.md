---
title: Topology Aware Routing
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Topology Aware Routing 是什么
- 如何 Topology Aware Routing
trigger_keywords:
- Topology
- Aware
- Routing
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Topology Aware Routing

## 概述

拓扑感知路由（Topology Aware Routing，旧称 Topology Aware Hints）是一种帮助将网络流量保留在其发起可用区（zone）内的机制。通过在 EndpointSlice 中为端点设置 zone 提示，kube-proxy 可优先将流量路由到同一拓扑区域的端点，从而降低网络延迟、提升可靠性并可能减少跨区流量成本。

## 核心概念/原理

- **拓扑提示（Hints）**：EndpointSlice 控制器在计算 [[Service|Service]] 的后端端点时，会考虑每个端点所在节点的拓扑信息（region 和 zone），并在 EndpointSlice 的 `hints.forZones` 字段中为端点分配提示。
- **kube-proxy 消费提示**：kube-proxy 在转发流量时，会根据自身所在 zone 过滤带有对应 zone 提示的端点，优先选择同 zone 端点。如果某个端点被分配到其他 zone，也会有少量跨区流量用于均衡负载。
- **按比例分配**：控制器默认根据各 zone 内节点的**可分配 CPU 核心数**比例来分配端点数量。例如，zone A 的可分配 CPU 是 zone B 的两倍，则 zone A 会分配到约两倍的端点提示。

## 关键机制或特性

- **启用方式**：通过在 Service 上添加注解 `service.[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]].io/topology-mode: Auto` 开启。在 Kubernetes 1.27 之前，使用旧注解 `service.kubernetes.io/topology-aware-hints`。
- **保护机制（Safeguards）**：当以下任一条件不满足时，系统会回退到全集群范围的路由，避免流量不均衡或黑洞：
  1. 端点数量少于集群 zone 数量。
  2. 无法在各区之间实现可接受的均衡分配（预期过载值超过阈值）。
  3. 有节点缺少 `topology.kubernetes.io/zone` 标签或可分配 CPU 信息。
  4. 部分端点缺少 zone hint（可能处于过渡状态）。
  5. kube-proxy 所在 zone 在 hints 中没有任何对应端点（常见于新增 zone 时）。
- **约束与限制**：
  - 不能与 `internalTrafficPolicy: Local` 同时使用。
  - 假设流量在各 zone 之间大致与节点容量成正比；若大部分流量来自单个 zone，可能导致该区端点过载。
  - EndpointSlice 控制器在计算比例时忽略未就绪节点以及带有 `node-role.kubernetes.io/control-plane` / `master` 标签的节点。
  - 未考虑 Pod 的 tolerations，若工作负载仅调度到部分节点，可能导致分配不均。
  - 与 Horizontal Pod Autoscaler 配合时可能存在延迟或不准确，因为流量压力可能集中在特定 zone。

## 使用场景

- **多可用区部署优化**：在跨多个可用区部署的集群中，将 Pod 间通信限制在同 zone，降低跨 AZ 网络延迟和费用。
- **大数据/批处理流量本地化**：需要大量节点间通信的工作负载，通过同 zone 路由减少骨干网带宽占用。
- **提升故障隔离性**：将流量限制在 zone 内，可在单个 zone 问题时减少对其他 zone 的影响面。

## 最佳实践/注意事项

- **确保每个 zone 有足够端点**：建议每个 zone 至少有 3 个端点，否则控制器很可能无法均匀分配 hints 并回退到全集群路由。
- **流量分布需相对均匀**：该特性不适合流量高度集中在某一个或少数 zone 的服务，否则会导致局部端点过载。
  - 若存在此类情况，建议评估使用 `trafficDistribution` 字段作为更灵活的替代方案。
- **避免与 internalTrafficPolicy: Local 混用**：同一 Service 上不能同时开启 topology-mode 和 `internalTrafficPolicy: Local`，但可以在集群中为不同 Service 分别使用。
- **关注控制器和节点标签**：确保所有工作节点都正确设置了 zone 标签，并且控制平面节点若运行工作负载也纳入考量。
- **监控与回退**：应监控 Service 的 EndpointSlice hints 设置情况，及时发现因 safeguards 触发导致的回退行为。

## 生产 YAML 示例

### 启用拓扑感知路由

```yaml
apiVersion: v1
kind: Service
metadata:
  name: api-backend
  namespace: production
  annotations:
    service.kubernetes.io/topology-mode: Auto     # 启用拓扑感知路由
spec:
  selector:
    app: api-backend
  ports:
  - port: 80
    targetPort: 8080
  # 注意：不能同时设置 internalTrafficPolicy: Local
```

### 确保每个可用区有足够端点

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-backend
  namespace: production
spec:
  replicas: 9                      # 3 个 zone × 每 zone 至少 3 个
  selector:
    matchLabels:
      app: api-backend
  template:
    metadata:
      labels:
        app: api-backend
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: api-backend
      containers:
      - name: api
        image: registry.example.com/apps/api:v3.0
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| EndpointSlice 无 hints | Safeguard 触发回退 | `kubectl get endpointslice -l kubernetes.io/service-name=<svc> -o yaml` 检查 hints 字段 |
| 流量仍跨可用区 | 端点数 < zone 数或节点缺少 zone 标签 | `kubectl get nodes --show-labels \| grep zone` |
| 局部端点过载 | 流量集中在某个 zone | 评估使用 `trafficDistribution` 替代 |
| 与 internalTrafficPolicy 冲突 | 同一 Service 同时配置了两者 | 移除其中一个配置 |

## 生产检查清单

- [ ] 所有工作节点设置 `topology.kubernetes.io/zone` 标签
- [ ] 每个 zone 至少 3 个端点（否则可能回退到全集群路由）
- [ ] 不与 `internalTrafficPolicy: Local` 同时使用
- [ ] 使用 topologySpreadConstraints 保证 Pod 跨 zone 均匀分布
- [ ] 监控 EndpointSlice hints 设置情况

## 命令快速参考

```bash
# 查看节点 zone 标签
kubectl get nodes -L topology.kubernetes.io/zone

# 检查 EndpointSlice 的 hints
kubectl get endpointslice -l kubernetes.io/service-name=<svc> -o json | jq '.items[].endpoints[] | {address: .addresses[0], zone: .zone, hints: .hints}'

# 查看 Service 注解
kubectl get svc <name> -o jsonpath='{.metadata.annotations}'
```

## 交叉引用

- [[domain-17-system-foundation/topic-dictionary/networking/service-internal-traffic-policy.md|Service Internal Traffic Policy]]](service-internal-traffic-policy.md) — 节点本地路由（互斥特性）
- [[domain-17-system-foundation/topic-dictionary/networking/endpointslices.md|EndpointSlices]]](endpointslices.md) — hints 字段和 zone 信息
- [Service](service.md) — trafficDistribution 字段
- [Cluster Networking](cluster-networking.md) — 跨可用区流量优化

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/aeraki-mesh.md|Aeraki Mesh 七层网格]]
- [[domain-17-system-foundation/topic-dictionary/networking/akri.md|Akri 边缘设备发现]]
- [[domain-17-system-foundation/topic-dictionary/networking/antrea.md|Antrea 网络方案]]
