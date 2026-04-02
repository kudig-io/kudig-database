# EndpointSlices

## 概述

EndpointSlice 是 Kubernetes 自 v1.21 起稳定的 API，用于跟踪 Service 的后端网络端点（通常是 Pod 的 IP 地址）。它是旧版 Endpoints API 的演进，能够支撑大规模 Service（数千个后端 Pod），并高效地更新后端列表，是 kube-proxy 进行内部流量路由的权威数据来源。

## 核心概念/原理

- **切片（Slice）**：一个 EndpointSlice 对象代表某个 Service 后端端点的一个子集。控制平面按 IP 协议族、端口、Service 名称等维度将端点分组到不同的 Slice 中。
- **自动创建与维护**：对于定义了 selector 的 Service，EndpointSlice 控制器会自动创建和维护对应的 EndpointSlice，持续同步 Pod 的变化。
- **地址类型**：每个 EndpointSlice 只包含一种地址类型：`IPv4` 或 `IPv6`。双栈 Service 至少对应两个 EndpointSlice。
- **条件（Conditions）**：
  - `ready`：端点是否准备好接收流量（`serving && !terminating` 的快捷方式）。
  - `serving`：端点是否正在提供服务。
  - `terminating`：端点是否正在终止（Pod 收到删除时间戳时设置），在滚动更新期间可用于避免流量丢失。
- **拓扑信息**：每个端点可携带 `nodeName` 和 `zone`，用于支持拓扑感知路由等功能。

## 关键机制或特性

- **容量与分配策略**：默认每个 EndpointSlice 最多包含 100 个端点，可通过 `--max-endpoints-per-slice` 配置，最大支持 1000。控制平面优先减少更新次数（降低向所有节点传播的开销），而非追求每个 Slice 完全填满。
- **管理标签（managed-by）**：`endpointslice.kubernetes.io/managed-by` 标签标识 EndpointSlice 的管理者。控制平面管理器的值为 `endpointslice-controller.k8s.io`，自定义控制器或手动管理应使用唯一值。
- **所有权（Ownership）**：EndpointSlice 通常由对应的 Service 通过 ownerReference 拥有，并带有 `kubernetes.io/service-name` 标签，便于查询。
- **端点去重**：由于更新可能异步到达，同一端点可能短暂出现在多个 Slice 中。消费者（如 kube-proxy）必须聚合所有关联的 EndpointSlice 并去重。
- **EndpointSlice Mirroring（已弃用）**：为兼容旧版 Endpoints API，控制平面会将用户创建的 Endpoints 镜像为 EndpointSlice。该功能及 Endpoints API 均已弃用，建议直接创建 EndpointSlice。

## 使用场景

- **大规模 Service 后端管理**：当 Service 背后有数百至数千个 Pod 时，EndpointSlice 将端点拆分为多个对象，避免单个 API 对象过大。
- **kube-proxy 路由依据**：每个节点的 kube-proxy 监听 EndpointSlice 变化，维护本地路由规则。
- **自定义服务发现**：Service Mesh 或自定义控制器可直接消费 EndpointSlice，实现更精细的流量控制。
- **手动管理外部端点**：对于无 selector 的 Service，手动创建 EndpointSlice 可将流量转发到集群外部地址。

## 最佳实践/注意事项

- **优先使用 EndpointSlice API**：新开发或迁移工作应避免使用旧版 Endpoints API，以获得双栈支持、更大规模和更丰富的元数据。
- **手动创建时避免无效 IP**：EndpointSlice 中的地址不能是 loopback（127.0.0.0/8, ::1/128）、link-local（169.254.0.0/16, fe80::/64）或其他 Kubernetes Service 的 ClusterIP。
- **设置 managed-by 标签**：自定义工具或控制器管理 EndpointSlice 时，应设置合适的 `managed-by` 标签值，避免与系统控制器冲突。
- **客户端需聚合去重**：读取 EndpointSlice 的客户端必须遍历 Service 关联的所有 Slice，并合并去重，参考 `kube-proxy` 中的 `EndpointSliceCache` 实现。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/
