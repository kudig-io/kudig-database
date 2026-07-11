---
title: Kubernetes × Service
summary: Kubernetes 与 Service 的交叉：Service 如何作为 K8s 网络模型的核心抽象，将动态 Pod 编排与稳定的服务发现、负载均衡解耦。
category: synthesis
tags:
- k8s
- service
- networking
- service-discovery
- kube-proxy
- endpointslice
tier: supporting
sources:
- 系统基础/topic-dictionary/fundamentals/kubernetes.md
- 系统基础/topic-dictionary/networking/service.md
- concepts/service-networking.md
- concepts/bp-operations.md
- concepts/bp-infrastructure.md
- concepts/bp-observability.md
created: '2026-07-02'
updated: '2026-07-02'
last_updated: 2026-07-11
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.75
lifecycle: draft
lifecycle_changed: '2026-07-02'
---


# Kubernetes × Service

## The Connection

Kubernetes 的 Pod 是临时的——它们会被调度到不同节点、因故障被重建、因弹性伸缩被增减。如果应用之间直接通过 Pod IP 通信，每次 Pod 变更都需要客户端更新连接地址。Service 通过提供一个稳定的虚拟 IP（ClusterIP）和 DNS 名称，在 Pod 的动态性与消费者的稳定性需求之间建立了抽象层。Service 是 Kubernetes 网络模型的基石，几乎所有集群内外通信都经由它路由。从实现机制看，Service 的工作分为三个阶段：(1) 控制面根据 Service 的 selector 匹配 Pod 标签，自动维护 EndpointSlice 对象记录后端 Pod 的真实 IP+端口；(2) 每个节点上的 kube-proxy 监听 Service/EndpointSlice 变更，将 ClusterIP 的流量转发规则写入 iptables/IPVS/eBPF 数据面；(3) CoreDNS 将 `<service>.<namespace>.svc.cluster.local` 解析为 ClusterIP，客户端通过 DNS 获取稳定入口。这三层协作使得 Pod 的创建/销毁/迁移对消费者完全透明——EndpointSlice 实时更新，kube-proxy 规则自动刷新，客户端无需感知后端变化。^[inferred]

## Where They Co-occur

- **kube-proxy 实现 Service 路由**：每个节点上的 kube-proxy 监听 Service 和 EndpointSlice 变更，通过 iptables/IPVS/eBPF 规则将 ClusterIP 流量转发到后端 Pod
- **CoreDNS 提供服务发现**：Pod 通过 `<service>.<namespace>.svc.cluster.local` 域名解析到 Service ClusterIP，无需硬编码地址
- **EndpointSlice 控制器**：当 Pod 标签匹配 Service selector 时，控制面自动维护 EndpointSlice 对象，记录后端 Pod 的真实 IP 和端口
- **流量策略**：`externalTrafficPolicy: Local` 保留客户端源 IP，但可能导致流量不均衡；`Cluster` 模式均匀分发但丢失源 IP
- **Headless Service + StatefulSet**：为有状态应用（如数据库集群）提供每个 Pod 独立的 DNS 记录，支持有序发现和定向访问
- **Gateway API 演进**：Service 的 LoadBalancer 类型正逐步被 Gateway API 取代，后者提供更丰富的路由控制和多协议支持
- **Session Affinity**：`sessionAffinity: ClientIP` 让 kube-proxy 将同一客户端 IP 固定到同一后端 Pod，适合有状态会话保持，但可能导致负载不均
- **AppProtocol 标注**：Service port 的 `appProtocol` 字段（`http`/`grpc`/`kafka`）帮助 Service Mesh 和 Ingress Controller 自动识别协议
- **Topology Aware Routing**：`trafficDistribution` 字段让 Service 优先将流量路由到同 zone/region 的 Pod，降低跨区延迟
- **EndpointSlice 多维度**：每个 EndpointSlice 最多含 100 个 endpoint，包含 Pod IP、端口、拓扑信息（zone/node）和 conditions（ready/serving/terminating），大规模集群中替代了旧版 Endpoints 对象
- **Internal Traffic Policy**：`internalTrafficPolicy: Local` 限制 Service 流量仅在 Pod 所在节点转发，避免跨节点 SNAT 导致的源 IP 丢失和额外网络跳数
- **Multi-port Service**：一个 Service 可暴露多个端口（如 `port: 8080` HTTP + `port: 9090` gRPC），每个端口可独立配置 `targetPort` 和 `appProtocol`——减少需要创建的 Service 数量
- **Service readiness 与 EndpointSlice conditions**：EndpointSlice 的 `conditions.ready`、`conditions.serving`、`conditions.terminating` 三态分离——Pod 进入 Terminating 时 ready=false 但 serving=true，允许 Ingress/kube-proxy 优雅排空流量

## Cross-cutting Insight

Kubernetes 解决了"应用如何在集群中运行"，Service 解决了"应用如何被找到和访问"。二者的结合使得微服务架构在 Kubernetes 上成为可能：每个微服务只需关心自己的 Service 名称，无需感知后端的 Pod 数量、IP 变化或节点分布。Service 将 Kubernetes 的动态编排能力转化为对消费者透明的稳定网络端点。更深层的价值在于"解耦生命周期"：Pod 的生命周期（创建→运行→销毁）与 Service 的生命周期（创建→稳定存在→删除）完全独立——Service 可以在后端 Pod 全部不可用时仍然存在（返回 503），也可以在后端 Pod 滚动更新时保持服务不中断（新旧 Pod 共存于 EndpointSlice）。这种"接口稳定 + 实现可变"的设计模式，正是微服务可独立部署、独立演进的基础。在实践中，Service 的稳定性也不是绝对的——EndpointSlice 的更新延迟（Pod Ready 到 EndpointSlice 同步通常有 1-5s 窗口）、kube-proxy 规则刷新延迟（大规模集群可能更长）以及 DNS 缓存 TTL 共同构成了 Service 的"收敛时间"。理解这一收敛窗口对于设计优雅停机（preStop hook + terminationGracePeriodSeconds）和金丝雀发布（流量切换的过渡期）至关重要。^[inferred]

## Tensions and Trade-offs

| 维度 | Kubernetes 编排侧 | Service 网络侧 | 结合注意事项 |
|---|---|---|---|
| Pod 生命周期 | 频繁创建/销毁 | EndpointSlice 需实时更新 | 大规模 Pod 变更可能导致 kube-proxy 规则刷新延迟 |
| 负载均衡 | 无原生应用层感知 | kube-proxy L4 轮询/最少连接 | 需要 L7 智能路由时须引入 Ingress/Service Mesh |
| 多集群 | Federation/多集群控制器 | Service 仅在本集群有效 | 跨集群服务发现需要额外方案（如 MCS API） |
| 网络策略 | Namespace/Pod 级隔离 | Service 本身不执行访问控制 | 需配合 NetworkPolicy 或 Service Mesh 实现零信任 |
| 外部访问 | NodePort/LoadBalancer | 云厂商 LB 集成 | LoadBalancer IP 字段已弃用，转向 Gateway API |
| 服务发现延迟 | EndpointSlice 秒级更新 | DNS 缓存可能掩盖变化 | 客户端需合理设 DNS TTL + 连接池 |

## Open Questions

- 大规模集群（>5000 Service）中 iptables 规则爆炸问题，IPVS 和 eBPF (Cilium) 的性能对比如何？
- 在混合云/多云场景下，如何统一管理集群内 Service 和外部 legacy 系统的服务发现？MCS API（Multi-Cluster Services）能否成为标准？
- Gateway API 全面替代 Service LoadBalancer 类型的时间线和迁移路径是什么？
- EndpointSlice 更新延迟（Pod Ready → Endpoint 同步）在大规模集群中如何优化？是否需要 topology-aware routing？

## Related

- [[系统基础/知识字典/fundamentals/kubernetes.md|Kubernetes]]
- [[系统基础/知识字典/networking/service.md|Service]]
- [[概念/service-networking.md|Service Networking]]
- [[概念/bp-operations.md|最佳实践：Operations]]
- [[概念/bp-infrastructure.md|最佳实践：Infrastructure]]
- [[综合/service-ingress.md|Service × Ingress]]
- [[综合/kubernetes-etcd.md|Kubernetes × etcd]]


<!-- risk-assessed -->
