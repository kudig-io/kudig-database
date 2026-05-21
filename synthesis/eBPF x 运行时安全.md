---
title: eBPF x 运行时安全
description: 'title: eBPF x 运行时安全'
category: general
tags:
- k8s
- cilium
- falco
- kafka
- networkpolicy
- ebpf
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- eBPF x 运行时安全 是什么
- 如何 eBPF x 运行时安全
trigger_keywords:
- eBPF
- 运行时安全
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- kafka-basics
---

---
title: eBPF x 运行时安全
category: synthesis
tags:
- k8s
- ebpf
- security
- cilium
- tetragon
- falco
- runtime-security
- networking
sources:
- concepts/cilium-ebpf-networking.md
- entities/tetragon.md
- entities/falco.md
- entities/cilium.md
- concepts/security-defense-depth.md
- concepts/service-mesh-architecture.md
created: 2026-05-21 14:00:00+00:00
updated: 2026-05-21 14:00:00+00:00
summary: "eBPF 如何将网络、可观测性和运行时安全三大安全关注点统一为单一内核级数据面——从 Cilium 的 L7 策略到 Tetragon 的进程追踪，揭示了一种统一的内核级安全模型。"
provenance:
  extracted: 0.2
  inferred: 0.7
  ambiguous: 0.1
base_confidence: 0.88
lifecycle: draft
lifecycle_changed: 2026-05-21

tier: supporting---

# eBPF x 运行时安全

## 连接点

eBPF 从根本上改变了 Kubernetes 的安全格局——它将传统上相互独立的三大关注点**网络**、**可观测性**和**运行时安全**——统一到一个内核级数据面中。在 eBPF 之前，这些由互不关联的工具各自处理：iptables 负责网络策略、独立 agent 负责指标采集、另一套运行时安全监控负责进程审计。eBPF 让这三者共享相同的内核挂载点、相同的 eBPF Map 和相同的策略引擎，创造出用户态工具无法实现的整体安全态势。

[[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] 描述了 eBPF 在网络和 L7 策略中的应用，[[entities/tetragon.md|tetragon]] 描述了 eBPF 在运行时安全中的进程追踪能力，[[falco]] 描述了基于内核模块/ eBPF 的运行时安全检测，但 wiki 中没有一页明确指出：**eBPF 创造了一个"安全连续体"，网络策略违规和运行时安全事件之间的边界是人为划分的。**

## 共现场景

这两个概念在 wiki 的网络、安全和可观测性域中反复共现，关键交汇点包括：

- **Cilium** 用 eBPF 实现 L3/L4/L7 网络策略（网络）+ 服务网格 mTLS（安全）+ 流量可视化（可观测性）——三合一
- **Tetragon** 使用 eBPF kprobe 和 cgroup 挂钩实现进程执行监控、文件访问追踪和网络异常检测——与 Cilium 共享相同的 eBPF 基础设施
- **Hubble** 使用与 Tetragon 相同的 eBPF socket 和 TC 挂载点，但用于流可观测性而非强制 enforcement
- **Cilium Service Mesh** 用 eBPF L4 mTLS 取代 sidecar 代理模式，消除了每 Pod 代理开销

这三个工具（Cilium、Tetragon、Hubble）被 wiki 分别视为"网络"、"安全"和"可观测性"的独立页面，但它们实际上是同一个 eBPF 基础设施在不同层面的应用——同一个 Cilium Agent 同时将 eBPF 程序注入内核，分别服务这三个目标。

## 交叉洞察

**核心洞察：eBPF 创造了一个"安全连续体"，传统意义上的"网络策略违规"和"运行时安全事件"之间的界限变得人为化。**

一个 eBPF 程序可以同时做到：

1. **强制** NetworkPolicy（L3/L4 丢包）
2. **解析** socket 层的 HTTP 头部（L7 策略）
3. **追踪** 发起该连接的进程（execve 的 kprobe）
4. **记录** 该流到 Hubble 用于服务依赖映射
5. **告警** Tetragon 如果该进程匹配已知攻击模式

这意味着运行 Cilium + Tetragon 的组织不是在部署"网络 + 安全"两套工具，而是在部署一个**统一的 eBPF 安全结构**，它同时在多个层面运作。区分"网络策略问题"和"运行时安全事件"变成了"哪个 eBPF 程序被触发"的问题，而不是"哪个工具检测到"的问题。

**性能意义：** eBPF 的近零开销（mTLS 延迟成本 <1%，内存 <10MB）使得在每个层面强制执行安全策略成为可能，而不会付出过去用户态工具带来的性能代价。这正是 eBPF 安全能做到而用户态工具做不到的关键：在全面监控的同时不产生令人望而却步的资源开销。

**与 Falco 的对比：** Falco 传统上使用内核模块，后来增加了 eBPF 支持。Tetragon 从第一天就基于 eBPF 构建。两者的区别在于：Tetragon 更精确（进程级追踪、eBPF 映射），Falco 规则集更丰富（200+ 内置规则）。在 eBPF 安全连续体中，Tetragon 是"精确打击"，Falco 是"全面扫描"。

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **内核版本锁定** | eBPF 功能需要特定内核版本（5.10+ 基础功能，6.1+ 高级功能）。使用旧内核的组织（如某些云厂商托管节点）无法使用高级 Tetragon 功能，除非升级内核。 |
| **eBPF 验证器约束** | eBPF 验证器拒绝带有无界循环或复杂逻辑的程序。这限制了 Tetragon 在内核中可以检测的复杂攻击模式——某些复杂攻击需要用户态关联，造成内核级检测和用户态检测之间的盲区。 |
| **Cilium 与 Falco 重叠** | Tetragon（eBPF）和 Falco（内核模块/eBPF）都提供运行时安全。同时运行两者会产生冗余和潜在的 eBPF Map 冲突。组织需要在精确度（Tetragon）和规则覆盖度（Falco）之间做出选择。 |
| **策略复杂度** | CiliumNetworkPolicy 的 L7 能力（HTTP 路径匹配、Kafka 主题）很强大但带来运维复杂度。配置错误的 L7 策略会静默阻断合法流量，且排障难度远高于传统 L3/L4 规则。 |
| **可观测性开销** | 虽然 eBPF 本身很轻量，但在大规模下启用完整 Hubble 流日志会产生显著的存储成本。需要在全面可见性和可控日志量之间做权衡。 |

## 开放问题

- **eBPF 程序版本管理：** 如何安全地跨集群升级 eBPF 程序，而不引发内核恐慌或在滚动升级期间产生策略盲区？
- **多租户 eBPF 策略：** 在多租户集群中，如何确保一个租户的 eBPF 程序不会干扰另一个租户的策略？
- **回退策略：** 如果 eBPF 数据面故障（eBPF 程序导致内核恐慌），安全回退是什么——数据包是被丢弃还是放行？wiki 尚未全面覆盖 eBPF 故障模式。
- **Tetragon 的策略即代码：** CiliumNetworkPolicy 已有成熟的 GitOps 工作流，但 Tetragon 的 TracingPolicy 尚缺乏等效的标准化策略即代码模式，无法纳入 GitOps 管道。
- **eBPF 安全成熟度评估：** 如何量化评估一个集群的 eBPF 安全覆盖率？哪些 eBPF 程序已部署、哪些内核挂载点已利用、哪些安全层已通过 eBPF 实现？

## 相关

- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]]
- [[cilium]]
- [[entities/tetragon.md|tetragon]]
- [[falco]]
- entities/hubble

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## Related

- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
