---
title: Service × Ingress
summary: Service 与 Ingress 的交叉：从集群内部服务发现到外部流量接入的完整网络路径，以及 Gateway API 对二者关系的重塑。
category: synthesis
tags:
- service
- ingress
- networking
- gateway-api
- load-balancing
- tls
tier: supporting
sources:
- 系统基础/topic-dictionary/networking/service.md
- 系统基础/topic-dictionary/networking/ingress.md
- concepts/service-networking.md
- concepts/bp-operations.md
- concepts/bp-security.md
created: '2026-07-02'
updated: '2026-07-02'
last_updated: 2026-07-11
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.73
lifecycle: draft
lifecycle_changed: '2026-07-02'
---


# Service × Ingress

## The Connection

Service 负责集群内部的服务发现和负载均衡，提供稳定的 ClusterIP 端点；Ingress 负责将集群外部的 HTTP/HTTPS 流量路由到内部的 Service。二者构成 Kubernetes 网络模型的"内外两层"：Service 是 Ingress 的后端目标，Ingress 是 Service 的外部入口。没有 Service，Ingress 没有流量目的地；没有 Ingress（或 NodePort/LoadBalancer），Service 只能在集群内访问。从流量路径看，一个外部请求的完整链路是：DNS → 外部 LB（云厂商提供）→ Ingress Controller Pod（Nginx/Envoy/Traefik）→ Ingress 规则匹配（Host + Path）→ 目标 Service ClusterIP → kube-proxy/EndpointSlice → 后端 Pod。在这条链路中，Service 的角色是"稳定的虚拟后端"——Ingress Controller 不直接连 Pod IP（因为 Pod IP 会变），而是通过 Service 的 EndpointSlice 获取可用的后端 Pod 列表。这意味着 Ingress Controller 需要实时感知 EndpointSlice 变更（通过 watch API），以便在后端 Pod 滚动更新时无缝切换流量。理解这条完整链路对于排障至关重要——一个"503 Service Unavailable"可能源于 EndpointSlice 为空（Service selector 不匹配）、Ingress 规则拼写错误、或 Ingress Controller 自身 Pod 资源不足。^[inferred]

## Where They Co-occur

- **Ingress 规则指向 Service 端口**：每条 Ingress rule 的 `backend` 引用一个同 Namespace 的 Service 及其端口号，Ingress Controller 将匹配流量转发到该 Service 的 EndpointSlice
- **TLS 终止分层**：TLS 在 Ingress 层终止（证书绑定在 Ingress Secret），Ingress 到 Service 的流量为明文；端到端加密需在 Service/Pod 层额外配置
- **Ingress Controller 的 Service 类型**：Ingress Controller 本身通常以 Deployment 运行，通过 `type: LoadBalancer` 或 `type: NodePort` 的 Service 暴露到集群外部
- **Gateway API 统一模型**：Gateway API 将 Ingress 和 Service 的关系重新抽象为 Gateway → HTTPRoute → Service 的三层结构，支持更细粒度的路由控制（如权重、Header 匹配、请求镜像）
- **Service Mesh 介入**：当引入 Istio/Linkerd 等 Service Mesh 时，Ingress Gateway 替代传统 Ingress Controller，Service 间的流量被 sidecar 代理拦截，实现 mTLS 和细粒度流量治理
- **健康检查联动**：Ingress Controller 的健康检查通常针对后端 Pod，而非 Service 本身；当 Service 的 EndpointSlice 为空时，Ingress 返回 503

## Cross-cutting Insight

Service 和 Ingress 共同定义了 Kubernetes 的"网络边界"：Service 是内部边界（谁可以调用我的微服务），Ingress 是外部边界（外部用户如何到达我的应用）。在生产环境中，运维团队需要同时理解两层的路由行为才能完成端到端的故障排查——一个"Service 不通"的工单，根因可能在 Ingress 的 TLS 证书过期、Ingress Controller 的 Pod 资源不足，或后端 Service 的 EndpointSlice 未就绪。更深层地看，Service 和 Ingress 的职责边界正在被 Gateway API 重新定义。在传统模型中，Ingress API 只支持 Host + Path 路由，复杂流量策略（权重、Header 匹配、请求镜像、流量切分）需要大量厂商特定的 annotation（如 `nginx.ingress.kubernetes.io/canary-weight`），导致 Ingress 配置不可移植。Gateway API 将"网关基础设施"（Gateway）、"路由规则"（HTTPRoute）和"后端服务"（Service）拆分为独立角色——平台团队管 Gateway，应用团队管 HTTPRoute，边界清晰且可跨实现移植（同一 HTTPRoute 可在 Nginx/Envoy/Traefik 上工作）。这种角色拆分使得 Service 和"入口"的关系从"硬编码在 annotation 中"升级为"声明式 API 表达"，是 K8s 网络模型走向成熟的重要一步。^[inferred]

## Tensions and Trade-offs

| 维度 | Service 独立使用 | Ingress 独立使用 | 结合注意事项 |
|---|---|---|---|
| 协议支持 | L4（TCP/UDP/SCTP） | L7（HTTP/HTTPS only） | 非 HTTP 流量需 NodePort/LoadBalancer |
| 路由能力 | 无（轮询/最少连接） | Host + Path 路由 | 高级路由（权重/Header）需 Gateway API |
| TLS | 不处理 | Ingress 层终止 | 端到端加密需额外配置 |
| 外部暴露 | NodePort/LoadBalancer | Ingress Controller | 多层 LB 可能导致源 IP 丢失 |
| API 演进 | 稳定 | 已冻结，推荐 Gateway API | 迁移 Ingress → Gateway API 需评估兼容性 |
| 故障域 | Service/EndpointSlice | Ingress Controller Pod | 需分别监控两层的健康状态 |
| 客户端 IP 保留 | `externalTrafficPolicy: Local` | Ingress 可能重写来源 | 多层 LB 叠加导致源 IP 丢失 |

## Open Questions

- Gateway API 完全替代 Ingress API 的时间表是什么？现有 Ingress 资源如何平滑迁移？是否需要 dual-write 过渡期？
- 在专有云环境中，如何在无外部 LoadBalancer 的情况下实现 Ingress Controller 的高可用外部接入？MetalBGP + Keepalived 是否足够？
- Service Mesh 场景下，Ingress Gateway 与传统 Ingress Controller 的职责边界如何划分？Ingress Gateway 是否应替代 Ingress Controller？
- 当多层负载均衡（外部 LB → Ingress Controller → Service → Pod）叠加时，源 IP 丢失问题如何系统性解决？PROXY protocol 是否是标准方案？

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[22-概念/03-网络/service-networking.md|Service Networking]]
- [[22-概念/10-最佳实践/bp-operations.md|最佳实践：Operations]]
- [[22-概念/10-最佳实践/bp-security.md|最佳实践：Security]]
- [[24-综合/03-网络与服务网格/kubernetes-service.md|Kubernetes × Service]]


<!-- risk-assessed -->
