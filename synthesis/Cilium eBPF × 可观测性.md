---
title: Cilium eBPF × 可观测性
description: '[[concepts/cilium-ebpf-networking]] 描述 eBPF 在网络中的应用，[[entities/prometheus-grafana]] 是监控栈。两者的交汇点是 **Hubble**：Cilium
  的可观测性子系统，它使用与网络策略相同的 eBPF 挂载点来收集流数据，无需额外的 sidecar 或代理。wiki 分别讨论了 eBPF 网络和 Prometheus '
category: synthesis
tags:
- k8s
- cilium
- ebpf
- observability
- networking
- hubble
- prometheus
- grafana
- istio
- envoy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium eBPF × 可观测性 是什么
- 如何 Cilium eBPF × 可观测性
trigger_keywords:
- Cilium
- eBPF
- 可观测性
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- kafka-basics
- observability-basics
created: "2026-05-23"
relationships:
  - target: "[[entities/deployment]]"
    type: uses
  - target: "[[entities/istio]]"
    type: uses
  - target: "[[entities/prometheus]]"
    type: uses
  - target: "[[entities/cni]]"
    type: related_to
---

# Cilium eBPF × 可观测性


## 连接点

[[concepts/cilium-ebpf-networking]] 描述 eBPF 在网络中的应用，[[entities/prometheus|prometheus]]-grafana]] 是监控栈。两者的交汇点是 **Hubble**：Cilium 的可观测性子系统，它使用与网络策略相同的 eBPF 挂载点来收集流数据，无需额外的 sidecar 或代理。wiki 分别讨论了 eBPF 网络和 Prometheus 监控，但没有指出 **eBPF 正在将网络可观测性从"采样推断"转变为"全量捕获"**。

## 共现场景

- **Hubble 指标**：Cilium 的 eBPF 程序在数据包经过时自动记录流信息（源/目标 IP、端口、协议、HTTP 头部），导出为 Prometheus 指标——无需应用修改或 sidecar 注入
- **L7 可观测性**：传统网络监控只能看到 L3/L4（IP/端口）。Cilium eBPF 可以在内核中解析 HTTP、gRPC、Kafka 协议，导出 L7 延迟和错误率——这是 sidecar 服务网格的功能，但没有 sidecar 的开销
- **网络策略命中可视化**：Hubble 可以显示哪些 NetworkPolicy 规则被触发、哪些流量被丢弃——将网络策略的可观测性与流量可视化结合
- **DNS 可观测性**：Cilium 通过 eBPF 拦截 DNS 查询，导出 DNS 请求/响应指标（包括 NXDOMAIN、超时）——这是传统监控难以实现的

## 交叉洞察

**核心洞察：eBPF 使网络可观测性从"尽力而为的采样"升级为"内核级全量捕获"，且成本趋近于零。**

传统网络监控的困境：
- **Sidecar 模式**：[[entities/istio|Istio]] Envoy 提供丰富的 L7 指标，但每个 Pod 增加 ~100MB 内存和 ~5% CPU
- **Node Agent 模式**：Prometheus node-exporter 采集节点级指标，但无法感知 Pod 级网络流
- **Flow 采样**：NetFlow/sFlow 采样率通常为 1:1000 或更低，丢失大量短连接信息

eBPF 的突破性在于：**它在处理网络数据包的同一个 eBPF 程序中同时执行策略强制和可观测性采集**。这意味着：
- 采集不是额外的步骤，而是策略执行的副产品
- 没有用户态-内核态的数据复制开销
- 全量捕获而非采样（因为每个数据包都已经经过 eBPF 程序）

**性能对比：**

| 方案 | 每 Pod 内存 | L7 可见性 | 全量捕获 | 额外 CPU |
|------|-----------|----------|---------|---------|
| Istio Sidecar | ~100MB | 是 | 是 | ~5% |
| Node Exporter | 0 | 否 | 否 | <1% |
| Cilium eBPF | 0 | 是 | 是 | <1% |

> Cilium 的 L7 可观测性不需要 per-Pod sidecar——它在节点级的 eBPF 程序中完成协议解析，然后聚合为 Prometheus 指标。

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **eBPF 验证器限制** | eBPF 验证器拒绝无界循环和复杂逻辑，这限制了 Cilium 可以解析的协议复杂度。某些自定义协议或加密流量（TLS）无法在 eBPF 中解析，需要回退到用户态处理 |
| **Hubble 存储成本** | 全量流日志在大规模集群中产生海量数据。启用 Hubble 流日志后，存储成本可能超过 Prometheus 指标存储。需要采样或聚合策略 |
| **协议解析的准确性** | eBPF 中的 HTTP 解析是"尽力而为"的——它依赖于已知的协议模式，可能被非标准实现或分片数据包误导。这与 Envoy 的完整 HTTP 解析相比精度较低 |

## 开放问题

- **eBPF 可观测性的标准化**：Hubble 的指标格式是 Cilium 特有的。是否应该有一个跨 [[entities/cni|CNI]] 的 eBPF 网络可观测性标准（如 OpenTelemetry 的网络语义约定）？
- **加密流量的 eBPF 可观测性**：TLS 1.3 的加密使得 eBPF 无法解析应用层内容。Cilium 的 L7 可观测性在 mTLS 环境下是否失效？是否需要结合 SPIFFE 身份来替代内容解析？


## 相关

- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## See Also

- [[synthesis/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- [[synthesis/CRD × 可观测性.md|CRD × 可观测性]]
- [[entities/deployment|Deployment]] × Secret 管理.md|Deployment × Secret 管理]]
- [[synthesis/GitOps x 平台工程.md|GitOps x 平台工程]]
## Related

- [[synthesis/Deployment × Secret 管理|[[deployment]] × Secret 管理]]
