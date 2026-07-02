---
title: Domain-35 eBPF 技术 — 开源项目索引
description: '| **Cilium** | eBPF 网络、安全、可观测性 | CNCF Graduated | v1.17.0 | 21k+ | Apache-2.0
  |'
summary: '| **Cilium** | eBPF 网络、安全、可观测性 | CNCF Graduated | v1.17.0 | 21k+ | Apache-2.0
  |'
category: ebpf-technology
tags:
- k8s
- ebpf
- cilium
- networking
- observability
- prometheus
- grafana
- falco
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 网络工程师
- 内核工程师
estimated_read_time: 5min
intent_queries:
- Domain-35 eBPF 技术 — 开源项目索引 是什么
- 如何 Domain-35 eBPF 技术 — 开源项目索引
- Kubernetes 35 ebpf technology 最佳实践
trigger_keywords:
- Domain-35
- eBPF
- 技术
- 开源项目索引
- ebpf
- technology
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-35 eBPF 技术 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Cilium** | eBPF 网络、安全、可观测性 | CNCF Graduated | v1.17.0 | 21k+ | Apache-2.0 |
| **Falco** | eBPF 运行时安全 | CNCF Graduated | v0.41.0 | 7.5k+ | Apache-2.0 |
| **Pixie** | K8s 可观测性 (eBPF) | New Relic | v0.14.0 | 5.5k+ | Apache-2.0 |
| **Inspektor Gadget** | eBPF 排查工具集 | 社区 | v0.38.0 | 7k+ | Apache-2.0 |
| **Tetragon** | eBPF 安全可观测性 | Cilium | v1.3.0 | 4k+ | Apache-2.0 |
| **BCC** | eBPF 编译工具集 | IOVisor | v0.31.0 | 20k+ | Apache-2.0 |
| **bpftrace** | eBPF 高级追踪语言 | 社区 | v0.22.0 | 8k+ | Apache-2.0 |
| **libbpf** | BPF 加载库 (内核官方) | Linux 内核 | v1.5.0 | 2k+ | LGPL-2.1/BSD |
| **eBPF for Windows** | Windows eBPF 支持 | Microsoft | v0.21.0 | 4k+ | MIT |
| **Katran** | L4 负载均衡 (eBPF) | Meta | v0.6.0 | 5k+ | GPL-2.0 |
| **Raptor** | eBPF 恶意软件检测 | 社区 | v0.1.0 | 300+ | Apache-2.0 |
| **Hubble** | Cilium 网络可观测性 | Cilium | v1.17.0 | 4k+ | Apache-2.0 |
| **Parca** | 持续性能分析 (eBPF) | Polar Signals | v0.23.0 | 4k+ | Apache-2.0 |
| **Grafana Beyla** | eBPF 应用自动可观测性 | Grafana | v2.0.0 | 1k+ | Apache-2.0 |
| **Parca** | 持续性能分析 (eBPF) | Polar Signals | v0.23.0 | 4k+ | Apache-2.0 |
| **Caretta** | K8s 网络映射 (eBPF) | Groundcover | v1.0.0 | 2k+ | Apache-2.0 |
| **L3AF** | eBPF 应用框架 | Linux Foundation | v1.0.0 | 1k+ | Apache-2.0 |
| **ebpf_exporter** | eBPF Prometheus 指标 | Cloudflare | v2.0.0 | 1.5k+ | MIT |
| **KubeArmor** | 容器运行时安全 (eBPF/LSM) | Accuknox | v1.4.0 | 3k+ | Apache-2.0 |

---

## 参考链接

- [eBPF 官方文档](https://ebpf.io/what-is-ebpf/)
- [Cilium eBPF 指南](https://docs.cilium.io/en/stable/bpf/)
- [BCC 文档](https://github.com/iovisor/bcc/blob/master/docs/tutorial.md)
- [bpftrace 参考](https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md)

---

## Obsidian 相关文档

- domain-35-ebpf-technology MOC
- [[domain-03-networking-traffic/README.md|Domain 03: eBPF 技术体系 (eBPF Technology Stack)]]
- eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program T...
- eBPF Map 类型与数据结构 (eBPF Map Types and Data Structures)
- Cilium CNI 架构与部署 (Cilium CNI Architecture and Deployment)
- Cilium 网络策略 L3/L4/L7 (Cilium Network Policy L3/L4/L7)
- Cilium Service Mesh 无 Sidecar 架构 (Cilium Service Mesh Sideca...
- Tetragon 运行时安全 (Tetragon Runtime Security)
- Hubble 网络可观测性 (Hubble Network Observability)
- bcc 与 bpftrace 工具链 (bcc and bpftrace Tools)
- eBPF 性能优化实践 (eBPF Performance Optimization Practice)
- eBPF 安全应用案例 (eBPF Security Applications and Use Cases)


<!-- risk-assessed -->
