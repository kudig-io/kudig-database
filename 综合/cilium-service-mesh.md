---
title: Cilium × Service Mesh
summary: Cilium 与服务网格的交叉：基于 eBPF 的 sidecarless 网格相比传统 sidecar 网格的架构取舍。
category: synthesis
tags:
- cilium
- service-mesh
- ebpf
- sidecar
- istio
tier: supporting
sources:
- 实体/cilium.md
- 概念/cilium-ebpf-networking.md
- 概念/service-mesh-architecture.md
- 概念/service-mesh-evolution.md
- 概念/sidecar-containers.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.75
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# Cilium × Service Mesh

## The Connection

传统服务网格（Istio/Linkerd）通过每 Pod 注入 sidecar 代理拦截流量，实现 mTLS、流量管理与可观测性。Cilium 利用 eBPF 在内核态直接处理这些能力，无需 sidecar（sidecarless）。二者的分歧在于"执行点放哪"：sidecar 在用户态每个 Pod 一份代理，eBPF 在内核态每节点一份。Cilium Service Mesh 代表了网格从"应用旁挂"到"内核下沉"的演进方向。从数据面架构对比，sidecar 模式在每个 Pod 的网络栈中插入一个 Envoy 代理，出入流量都经过 sidecar 转发（多一跳用户态网络处理）；Cilium 模式则在内核网络栈的 hook 点（TC、XDP、cgroup/connect）直接处理流量，无需额外跳数。这意味着 sidecar 的资源开销（CPU/内存）与 Pod 数量线性增长（每 Pod 一个 Envoy，即使空闲也占 ~50-100MB 内存），而 Cilium 的开销是节点级的常数（每节点一个 eBPF 程序集）。在千 Pod 节点上，sidecar 的累计内存开销可达数十 GB，而 Cilium 的增量接近零。这种"O(Pod) vs O(Node)"的资源模型差异是 Cilium 在大规模场景下的核心优势。^[inferred]

## Where They Co-occur

- **mTLS**：Istio 用 sidecar 终止 TLS；Cilium 在节点内核用 eBPF + 证书轮换实现 mTLS，无需改 Pod。
- **L7 流量管理**：Cilium 嵌入 Envoy 作为 L7 代理（按需启用），在保留 eBPF 高效数据面的同时获得 L7 能力。
- **可观测性**：Cilium Hubble 直接从 eBPF 产出服务依赖图与 HTTP 指标，无需 sidecar 上报。
- **Cilium Mesh / Cluster Mesh**：跨集群服务发现与路由，用 eBPF 替代多套 sidecar 的复杂互联。
- **混合模式**：部分流量走 eBPF（L3/L4），部分走 Envoy（L7），按需权衡性能与功能。
- **Sidecarless 的陷阱**：绕过了 Pod 级代理，应用若直接发起非标连接可能不经过网格策略——如使用 `SO_REUSEPORT` 或 raw socket 的应用可能绕过 eBPF hook。
- **Cilium L7 policy (Envoy on demand)**：Cilium 在节点上按需启动 Envoy 实例处理 L7 HTTP 规则，而非每 Pod 一个 sidecar——Envoy 以节点级共享模式运行，资源开销收敛到 O(Node)。
- **CiliumExternalWorkload**：Cilium 支持将非 K8s 虚拟机接入网格，通过 agent 注入实现与 K8s Pod 统一的 mTLS 身份和策略管控。

## Cross-cutting Insight

Sidecar 网格的价值在于"每 Pod 独立、可逐服务治理"，代价是资源与延迟开销线性增长；Cilium 的价值在于"节点级共享、零额外跳数"，代价是 L7 能力依赖内核态 Envoy 集成与策略粒度收敛。选择不是"谁取代谁"，而是"工作负载画像决定执行点"——大规模东西向 L4 场景 eBPF 更经济，复杂 L7 策略场景 sidecar 仍更成熟。更深层地看，两种模式的可运维性差异决定了它们的适用场景：sidecar 模式的升级需要重建 Pod（替换 sidecar 镜像触发滚动重启），在大规模集群中一次 Istio 升级可能涉及数千 Pod 重启，持续数小时并影响 SLO；Cilium 模式的升级只需替换节点的 eBPF 程序（通过 DaemonSet 更新 Cilium agent），Pod 本身不重启，对应用完全透明。但 Cilium 的代价是更强的内核版本耦合——eBPF 程序依赖特定内核版本的 BTF 信息，内核大版本升级可能导致网格功能降级。此外，sidecar 模式的"可观测性"更直观（Envoy access log 直接关联到 Pod），而 Cilium 的内核态观测需要通过 Hubble 从 eBPF maps 中导出，虽然无需 sidecar 但调试链路更长。因此在实际选型中，"运维预算"往往比"性能指标"更决定性——拥有专门 eBPF 平台工程能力的团队更适合 Cilium，而追求成熟度和文档丰富度的团队更安全的选择是 Istio sidecar。^[inferred]

## Tensions and Trade-offs

| 维度 | Sidecar 网格 (Istio) | Cilium eBPF 网格 | 结合注意事项 |
|---|---|---|---|
| 资源开销 | 每 Pod 一个代理，线性增长 | 每节点一份，收敛 | 大规模 eBPF 更省 |
| 延迟 | 多一跳用户态转发 | 内核态直通，跳数少 | 延迟敏感优先 eBPF |
| L7 能力 | Envoy 全功能，最成熟 | 内核 Envoy 集成，功能收敛 | 复杂 L7 仍倾向 sidecar |
| 升级 | 改 sidecar 镜像，需 Pod 重启 | 改节点 eBPF/Cilium，Pod 不动 | eBPF 升级对应用更透明 |
| 绕过风险 | Pod 出口必经 sidecar | 非 CNI 流量可能绕过 | 需 egress 策略兜底 |
| 生态成熟度 | 极成熟，文档/案例丰富 | 较新，复杂场景仍在完善 | 关键路径需评估稳定性 |
| 升级影响 | Pod 级重启（滚动更新） | 节点级更新（Pod 不动） | eBPF 升级对应用更透明 |

## Open Questions

- 当 L7 策略复杂到必须用 Envoy 时，Cilium 的"内核 Envoy"与 Istio 的"sidecar Envoy"在性能与可维护性上的真实差距？Envoy 配置热加载在两种模式下是否一致？
- sidecarless 模式下，如何防止恶意/异常 Pod 通过 raw socket 或 `SO_REUSEPORT` 绕过网格策略？是否需要 egress 策略兜底？
- 多集群 Cilium Mesh 与 Istio Multi-Primary 在跨集群 mTLS 与流量转移上的取舍如何量化？Cluster Mesh 的网络延迟是否可接受？
- Cilium Service Mesh 的 L7 能力（Envoy on demand）在生产中与 Istio 的完整 Envoy 功能矩阵相比有哪些已知差距？

## Related

- [[实体/cilium.md|Cilium]]
- [[实体/istio.md|Istio]]
- [[实体/linkerd.md|Linkerd]]
- [[实体/envoy.md|Envoy]]
- [[概念/service-mesh-architecture.md|服务网格架构]]
- [[概念/service-mesh-evolution.md|服务网格演进]]
- [[概念/sidecar-containers.md|Sidecar 容器]]
- [[概念/cilium-ebpf-networking.md|Cilium eBPF 网络]]
- [[综合/ebpf-observability.md|eBPF × Observability]]
- [[综合/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]]


<!-- risk-assessed -->
