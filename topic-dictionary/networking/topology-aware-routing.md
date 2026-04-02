# Topology Aware Routing

## 概述

拓扑感知路由（Topology Aware Routing，旧称 Topology Aware Hints）是一种帮助将网络流量保留在其发起可用区（zone）内的机制。通过在 EndpointSlice 中为端点设置 zone 提示，kube-proxy 可优先将流量路由到同一拓扑区域的端点，从而降低网络延迟、提升可靠性并可能减少跨区流量成本。

## 核心概念/原理

- **拓扑提示（Hints）**：EndpointSlice 控制器在计算 Service 的后端端点时，会考虑每个端点所在节点的拓扑信息（region 和 zone），并在 EndpointSlice 的 `hints.forZones` 字段中为端点分配提示。
- **kube-proxy 消费提示**：kube-proxy 在转发流量时，会根据自身所在 zone 过滤带有对应 zone 提示的端点，优先选择同 zone 端点。如果某个端点被分配到其他 zone，也会有少量跨区流量用于均衡负载。
- **按比例分配**：控制器默认根据各 zone 内节点的**可分配 CPU 核心数**比例来分配端点数量。例如，zone A 的可分配 CPU 是 zone B 的两倍，则 zone A 会分配到约两倍的端点提示。

## 关键机制或特性

- **启用方式**：通过在 Service 上添加注解 `service.kubernetes.io/topology-mode: Auto` 开启。在 Kubernetes 1.27 之前，使用旧注解 `service.kubernetes.io/topology-aware-hints`。
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
- **提升故障隔离性**：将流量限制在 zone 内，可在单个 zone 故障时减少对其他 zone 的影响面。

## 最佳实践/注意事项

- **确保每个 zone 有足够端点**：建议每个 zone 至少有 3 个端点，否则控制器很可能无法均匀分配 hints 并回退到全集群路由。
- **流量分布需相对均匀**：该特性不适合流量高度集中在某一个或少数 zone 的服务，否则会导致局部端点过载。
  - 若存在此类情况，建议评估使用 `trafficDistribution` 字段作为更灵活的替代方案。
- **避免与 internalTrafficPolicy: Local 混用**：同一 Service 上不能同时开启 topology-mode 和 `internalTrafficPolicy: Local`，但可以在集群中为不同 Service 分别使用。
- **关注控制器和节点标签**：确保所有工作节点都正确设置了 zone 标签，并且控制平面节点若运行工作负载也纳入考量。
- **监控与回退**：应监控 Service 的 EndpointSlice hints 设置情况，及时发现因 safeguards 触发导致的回退行为。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/
