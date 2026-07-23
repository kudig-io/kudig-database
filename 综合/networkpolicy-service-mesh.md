---
title: NetworkPolicy × Service Mesh
summary: NetworkPolicy 与服务网格的交叉：L3/L4 网络策略与 L7 网格策略的职责划分与互补。
category: synthesis
tags:
- networkpolicy
- service-mesh
- security
- l7-policy
- cilium
tier: supporting
sources:
- 概念/network-policy.md
- 概念/networkpolicy.md
- 概念/service-mesh-architecture.md
- 概念/service-mesh-zero-trust-security.md
- 实体/networkpolicy.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.74
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# NetworkPolicy × Service Mesh

## The Connection

Kubernetes NetworkPolicy 在 L3/L4（IP/端口）控制 Pod 间连通性，服务网格（Istio/Linkerd/Cilium Service Mesh）在 L7（HTTP/gRPC 方法、JWT、mTLS）控制服务间授权。两者都回答"谁可以访问谁"，但粒度与执行点不同：NetworkPolicy 在 CNI 数据面执行（iptables/eBPF），服务网格在 sidecar/eBPF 数据面执行（Envoy）。真正的零信任架构需要两者叠加——L4 做网络层兜底，L7 做应用层精细授权。从攻击面分析，二者覆盖不同的威胁向量：NetworkPolicy 阻止的是"不该连的 Pod 发起了 TCP 连接"（如被攻陷的 Pod 扫描集群内网），服务网格阻止的是"连上了之后不该执行的 API 调用"（如已认证的服务尝试调用非授权的 gRPC 方法）。一个典型的零信任策略栈是：默认 deny-all NetworkPolicy（只允许白名单 Pod 通信）→ mTLS 双向认证（确保通信双方身份可信）→ AuthorizationPolicy（基于身份限制可执行的 HTTP 方法/路径）。三层叠加构成纵深防御，任何一层被绕过都有下一层兜底。^[inferred]

## Where They Co-occur

- **默认拒绝 × 双层防御**：NetworkPolicy 实现 namespace/Pod 级默认 deny，服务网格 AuthorizationPolicy 实现 API 级允许，纵深防御。
- **Cilium 统一执行点**：Cilium 用 eBPF 同时实现 NetworkPolicy 与 L7 策略（CiliumNetworkPolicy + Envoy），消除 sidecar 也能做 L7 授权。
- **mTLS + NetworkPolicy**：网格提供传输加密与身份，NetworkPolicy 基于服务账户限制可达性，二者结合实现"加密 + 准入"。
- **eBPF 取代 sidecar**：Cilium Service Mesh 在内核态同时承载网络策略与 L7 观测，省去每 Pod 一个 Envoy。
- **故障排查重叠**：连接失败时，需同时排查 NetworkPolicy（被 deny）与 AuthorizationPolicy（被 RBAC 拒绝），口径易混淆——NetworkPolicy deny 表现为 TCP 超时（无应用层响应），AuthorizationPolicy deny 表现为 HTTP 403。
- **CiliumNetworkPolicy 扩展**：Cilium 在原生 NetworkPolicy 之上扩展了 `CiliumNetworkPolicy` CRD，支持 L7 规则（如 `HTTP` rules 匹配 method/path），在不需要 sidecar 的情况下实现 L7 授权。
- **身份 vs IP 策略**：传统 NetworkPolicy 基于 Pod IP/label，服务网格基于 SPIFFE 身份（`spiffe://cluster/ns/default/sa/myapp`）——身份比 IP 更稳定，但需要 mesh 基础设施支撑。
- **默认策略模式**：生产零信任最佳实践是 namespace 级 `default-deny-ingress` + `default-deny-egress` NetworkPolicy，再逐服务开白名单——"deny by default, allow by exception"。
- **Istio AuthorizationPolicy DENY 优先**：Istio 的 AuthorizationPolicy 支持 `DENY` action 且 DENY 优先于 ALLOW——先写默认 deny-all 规则，再逐 path 开 allow，实现最小权限语义。

## Cross-cutting Insight

NetworkPolicy 回答"这两个 Pod 能不能通"，服务网格回答"通了之后这个请求是否被允许"。把二者混为一谈是常见误区——仅靠 NetworkPolicy 无法实现"只允许 GET /api、需带 JWT"这类语义；仅靠服务网格，一旦 sidecar 被绕过（如 hostNetwork Pod）则形同虚设。分层授权、各司其职，才是可审计的零信任。更深层地看，生产环境中两层策略的"协同排障"是最大的运维痛点：当一个 API 请求返回 403 时，开发者无法判断是被 NetworkPolicy 在 TCP 层拒绝（连接超时/拒绝）还是被 AuthorizationPolicy 在 L7 层拒绝（HTTP 403 + RBAC deny 日志）。两种拒绝的表面现象不同——NetworkPolicy 通常表现为连接超时或 connection refused（因为没有应用层响应），而 AuthorizationPolicy 表现为明确的 HTTP 4xx 状态码。但在复杂链路中（如经过 Ingress Gateway → sidecar → backend），拒绝可能发生在任何一跳，定位根因需要逐跳排查。因此零信任架构的可运维性不取决于策略的严格程度，而取决于策略的"可观测性"——Cilium Hubble 能同时展示 L3/L4/L7 的流量决策日志，将 NetworkPolicy deny 和 AuthorizationPolicy reject 在同一个服务依赖图中标注，这是它相比"传统 CNI + Istio sidecar"组合的运维优势。^[inferred]

## Tensions and Trade-offs

| 维度 | NetworkPolicy (L3/L4) | Service Mesh (L7) | 结合注意事项 |
|---|---|---|---|
| 粒度 | IP/端口/命名空间 | HTTP 方法/路径/JWT/身份 | 需分清谁负责哪层 |
| 执行点 | CNI 数据面 | sidecar/eBPF | 绕过 sidecar 时 L7 失效 |
| 性能 | 内核态，开销小 | sidecar 增一跳，eBPF 较轻 | 大规模需权衡 |
| 加密 | 无（需额外方案） | 原生 mTLS | 网格承担传输安全 |
| 可观测 | 连接级日志 | 请求级遥测 | 排障需对齐两侧视图 |
| 默认策略 | 默认 allow，需显式 deny | 默认 allow，需显式 AuthorizationPolicy | 零信任需双向默认拒绝 |
| 策略可观测 | 连接级日志（有限） | 请求级遥测（Envoy access log） | 需统一排障视图避免归因混淆 |

## Open Questions

- 在 Cilium eBPF 模式下，NetworkPolicy 与 L7 策略同源执行，是否应废弃 sidecar 网格以简化排障？L7 能力差距何时弥合？
- 当一个服务既被 NetworkPolicy 又被 AuthorizationPolicy 拒绝时，如何让错误信息对开发者可定位？是否需要统一的 deny reason 标准？
- 多集群服务网格（如 Istio 多网络）下，跨集群 NetworkPolicy 该如何统一表达？MCS API 是否提供跨集群策略语义？
- 当 Pod 使用 `hostNetwork: true` 时绕过 CNI/sidecar，零信任策略应如何兜底？是否应禁止 hostNetwork Pod？

## Related

- [[实体/networkpolicy.md|NetworkPolicy]]
- [[实体/cilium.md|Cilium]]
- [[实体/istio.md|Istio]]
- [[实体/linkerd.md|Linkerd]]
- [[概念/network-policy.md|网络策略]]
- [[概念/network-policy.md|NetworkPolicy]]
- [[概念/service-mesh-architecture.md|服务网格架构]]
- [[概念/service-mesh-zero-trust-security.md|服务网格零信任]]
- [[概念/service-mesh-security-governance.md|服务网格安全治理]]
- [[综合/cilium-service-mesh.md|Cilium × Service Mesh]]
- [[综合/rbac-multitenancy.md|RBAC × Multi-tenancy]]


<!-- risk-assessed -->
