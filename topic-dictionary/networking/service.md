# Service

## 概述

Service 是 Kubernetes 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象。由于 Pod 是临时的、会被动态创建和销毁的，其 IP 地址也随之变化，Service 通过稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了前端客户端与后端 Pod 的耦合，使现有应用无需改造即可在 Kubernetes 中运行。

## 核心概念/原理

- **Selector 与 EndpointSlice**：Service 通过 `selector` 匹配标签相同的 Pod，控制平面自动创建并维护对应的 EndpointSlice，记录所有后端 Pod 的 IP 与端口。无 selector 的 Service 可配合手动创建的 EndpointSlice，将流量转发到集群外部地址或其他命名空间。
- **端口映射**：Service 的 `port` 是暴露的端口，`targetPort` 是 Pod 容器的实际监听端口，支持按名称引用容器端口，便于后端升级时平滑切换。
- **Headless Service**：将 `.spec.clusterIP` 显式设为 `"None"`，不再分配虚拟 IP，DNS 直接返回后端 Pod 的 IP 列表（A/AAAA 记录），适用于需要直接访问特定 Pod 或有状态服务场景。
- **服务发现**：集群内的 Pod 可通过环境变量（创建顺序有要求）或 DNS 发现 Service，推荐使用 DNS 方式以避免依赖启动顺序。

## 关键机制或特性

- **Service 类型**：
  - `ClusterIP`（默认）：集群内部可访问的虚拟 IP。
  - `NodePort`：在每个节点上开放固定端口（默认 30000–32767），将流量代理到 Service。
  - `LoadBalancer`：在云厂商环境中自动创建外部负载均衡器。
  - `ExternalName`：通过 DNS CNAME 将 Service 映射到外部域名，不做任何代理。
- **EndpointSlices**：自 v1.21 起稳定，是 kube-proxy 路由内部流量的真实来源，默认每个 Slice 最多 100 个端点（最大可配 1000）。旧版 Endpoints API 已弃用。
- **流量策略**：支持 `internalTrafficPolicy` 与 `externalTrafficPolicy`（Cluster/Local），控制流量在集群内部或外部进入时的路由范围。
- **会话保持（Session Affinity）**：可基于客户端 IP 配置会话亲和性，使同一客户端流量始终到达同一 Pod。
- **应用协议（appProtocol）**：自 v1.20 起稳定，用于为端口声明应用层协议（如 `kubernetes.io/h2c`、`kubernetes.io/ws`），供实现方提供更丰富的行为。

## 使用场景

- **微服务间通信**：通过 ClusterIP + DNS 实现服务间稳定调用。
- **外部访问入口**：使用 NodePort 或 LoadBalancer 将 Web 应用暴露到公网。
- **连接集群外服务**：利用无 selector Service + 手动 EndpointSlice 或 ExternalName 对接外部数据库、 legacy 系统。
- **有状态服务发现**：Headless Service 配合 StatefulSet，为每个 Pod 提供独立 DNS 记录。

## 最佳实践/注意事项

- **优先使用 DNS 发现**：相比环境变量，DNS 不依赖 Pod 与 Service 的创建顺序，更灵活可靠。
- **无 selector Service 需手动维护 EndpointSlice**：创建或更新 EndpointSlice 时，避免使用 loopback、link-local 或其他 Service 的 ClusterIP 作为 endpoint 地址。
- **NodePort 端口冲突**：可指定 `nodePort` 使用静态段（默认 30000–30085）以降低冲突概率；动态分配使用 30086–32767。
- **LoadBalancer IP 弃用**：`.spec.loadBalancerIP` 在 v1.24 已弃用，建议改用云厂商特定的注解或迁移到 Gateway API。
- **ExternalName 的协议兼容性**：对 HTTP/HTTPS 等依赖 Host 头的协议，ExternalName 可能导致 TLS 证书不匹配或 Host 头错误，需谨慎使用。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/service/
