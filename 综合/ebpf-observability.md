---
title: eBPF × Observability
summary: eBPF 与可观测性的交叉：如何通过内核态探针实现零插桩的指标、追踪与网络流量采集。
category: synthesis
tags:
- ebpf
- observability
- cilium
- tetragon
- pixie
tier: supporting
sources:
- 概念/cilium-ebpf-networking.md
- 概念/Cilium eBPF × 可观测性.md
- 概念/observability-pillars.md
- 概念/k8s-observability-stack.md
- 实体/cilium.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.76
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# eBPF × Observability

## The Connection

传统可观测性依赖应用侧插桩（SDK、agent、sidecar），成本高、覆盖不全、语言相关。eBPF 在 Linux 内核中运行沙箱程序，能在 syscall、网络套接字、内核探针（kprobe/uprobe）等位置采集数据，对应用完全透明。eBPF 让"全栈可观测性"从"逐服务改造"变成"集群一次性部署"，是云原生可观测性的底层使能器。从技术机制看，eBPF 程序通过 verifier 验证安全性后加载到内核，由 hook 点（tracepoint、kprobe、uprobe、perf_event、XDP）触发执行，采集的数据通过 ring buffer 或 BPF maps 传递到用户态导出器。这种"内核态探针 + 用户态导出"的架构使得 eBPF 可观测性工具无需修改应用代码、无需注入 sidecar、无需绑定特定语言运行时——只要内核版本支持（通常需要 Linux 4.18+ 和 BTF 信息），就能在 syscalls、网络栈、文件 I/O 层面获得全量、零采样的观测数据。CO-RE（Compile Once - Run Everywhere）机制进一步使 eBPF 程序可跨内核版本移植，降低了运维门槛。^[inferred]

## Where They Co-occur

- **Cilium Hubble**：基于 eBPF 采集 L3-L7 网络流量，生成服务依赖图（service map）与 HTTP/gRPC 指标，无需 sidecar 或应用 SDK；Hubble UI 提供实时流量拓扑可视化。
- **Pixie**：用 eBPF 直接采集 Python/Golang/Java 应用的 trace 与指标，无需手动埋点；基于 Pixie Function 可编写自定义采集脚本。
- **Tetragon**：eBPF 安全可观测性，实时检测进程执行、文件访问、网络连接等内核事件；支持声明式 TracingPolicy 定义安全事件规则。
- **Kepler**：通过 eBPF 读取 CPU/功耗计数器（RAPL），按 Pod 估算能耗，连接可观测性与 FinOps，实现"每个 Pod 花了多少电费"的度量。
- **Inspektor Gadget / bpfman**：提供调试与 eBPF 程序生命周期管理的工具链；bpfman 支持以 systemd 方式管理 eBPF 程序的加载与卸载。
- **网络指标 → Prometheus**：eBPF 采集的指标经 exporter 暴露，进入统一监控栈；Cilium 内置 Prometheus metrics endpoint，无需额外 exporter。
- **Kubescape + eBPF 运行时检测**：将 CIS benchmark 的静态合规检查与 Tetragon 的运行时行为检测结合，形成"构建时合规 + 运行时审计"的双层安全可观测性。
- **eBPF 替代 cAdvisor**：部分场景下 eBPF 可直接采集容器级 CPU/内存/网络指标，性能开销低于 cAdvisor 的 cgroups 轮询，适合超大规模集群。
- **bpftrace 即席查询**：`bpftrace` 工具支持编写单行 eBPF 脚本即席查询内核事件（如 `bpftrace -e 'tracepoint:syscalls:sys_enter_openat { @[comm] = count(); }'`），是高级 SRE 的内核级排障利器。
- **Cilium identity-aware metrics**：Cilium 的 eBPF 采集不仅包含网络五元组，还能关联 K8s Pod label/identity，直接产出"哪个 Service 调用了哪个 Service"的 L7 指标。
- **eBPF map 内存控制**：eBPF 程序使用 BPF maps 存储状态（如连接追踪表），大规模集群中 map 大小需通过 `--bpf-map-size` 调优，否则可能导致 map 满溢丢失事件。
- **Coroot / Parca eBPF profiling**：eBPF 持续性能分析（continuous profiling）工具——Parca 通过 eBPF 采集中断栈帧，Coroot 自动绘制服务调用图和延迟瀑布——无需代码插桩即可获得火焰图和 CPU profile。
- **eBPF 程序 verifier 安全性**：Linux 内核的 eBPF verifier 在加载时静态分析程序的控制流，确保无越界访问、无无限循环——这是 eBPF 被允许在生产内核中运行的安全前提，但也限制了部分复杂场景的表达能力。

## Cross-cutting Insight

eBPF 把可观测性的采集点从"应用边界"下移到"系统调用边界"。这一下移带来三个质变：覆盖度（任何语言、任何进程默认可观测）、保真度（内核态采集无采样盲区）、耦合度（采集逻辑不污染业务代码）。代价是可观测性栈与内核强耦合——内核升级可能使探针失效，运维责任从应用团队转移到了平台团队。更深层的挑战在于"语义鸿沟"：eBPF 采集的是系统级事件（syscall 号、网络五元组、进程 PID），而 SRE 排障需要的是业务级语义（用户 ID、订单状态、请求链路）。eBPF 可观测性的成熟度取决于能否将内核事件映射回业务上下文——例如 Tetragon 需要将 `execve` 事件关联到 Kubernetes Pod 标签，Hubble 需要将 TCP 连接映射到 Service 名称。只有当这层映射足够完整时，eBPF 才能从"内核调试工具"升级为"生产级可观测性平台"。^[inferred]

## Tensions and Trade-offs

| 维度 | 传统插桩可观测性 | eBPF 可观测性 | 结合注意事项 |
|---|---|---|---|
| 覆盖成本 | 逐服务改造，语言相关 | 集群一次部署，语言无关 | 混合栈需统一语义 |
| 语义丰富度 | 业务上下文（用户、订单）丰富 | 系统层语义（syscall/网络）强 | 业务 span 仍需少量插桩 |
| 内核耦合 | 与内核无关 | 强依赖内核版本/BTF | 需 CO-RE 与版本兼容矩阵 |
| 性能开销 | 在应用线程内 | 在内核探针，可控但需调参 | 高频事件需采样/过滤 |
| 安全权限 | 普通 Pod 即可 | 需特权/`CAP_BPF` | 多租户下需隔离策略 |
| 内核版本依赖 | 与内核无关 | 强依赖内核版本/BTF | 需维护兼容性矩阵与降级方案 |

## Open Questions

- 当 eBPF 探针与 Istio sidecar 同时采集网络指标时，如何避免重复计量与口径冲突？
- 多个 eBPF 工具（Cilium + Tetragon + Kepler）同节点共存时的资源预算应如何分配？是否存在 eBPF 程序数上限？
- 内核升级导致 BTF 不兼容时，可观测性采集中断的发现与回退机制该如何设计？是否应自动降级到传统 agent？
- eBPF 采集的内核事件如何与 OpenTelemetry trace 关联，实现"内核 syscall 异常 → 业务请求 span"的跨层排障？

## Related

- [[实体/cilium.md|Cilium]]
- [[实体/tetragon.md|Tetragon]]
- [[实体/pixie.md|Pixie]]
- [[实体/bpfman.md|bpfman]]
- [[实体/kepler.md|Kepler]]
- [[概念/cilium-ebpf-networking.md|Cilium eBPF 网络]]
- [[概念/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]]
- [[概念/eBPF × 运行时安全.md|eBPF × 运行时安全]]
- [[概念/observability-pillars.md|可观测性支柱]]
- [[概念/k8s-observability-stack.md|K8s 可观测性栈]]
- [[综合/cilium-service-mesh.md|Cilium × Service Mesh]]


<!-- risk-assessed -->
