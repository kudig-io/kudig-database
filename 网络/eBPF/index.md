---
title: eBPF Networking
description: eBPF 网络知识域 — eBPF 架构、Cilium CNI、网络策略、Service Mesh、Tetragon 安全、Hubble 可观测
category: subdomain
tags:
- ebpf
- cilium
- hubble
- tetragon
- bpftrace
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# eBPF 网络 eBPF Networking

> 基于 eBPF 的下一代云原生网络、安全与可观测性。

## eBPF 生态全景

| 项目 | 能力 | 场景 |
|------|------|------|
| Cilium | CNI + 网络策略 | Pod 网络与安全 |
| Hubble | 流量可视化 | 网络可观测 |
| Tetragon | 运行时安全 | 系统调用监控 |
| bpftrace | 动态追踪 | 性能调试 |
| BCC | 工具集 | 内核观测 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[网络/eBPF/01-ebpf-architecture-fundamentals.md\|eBPF 架构]] | 内核虚拟机/程序类型 | advanced |
| [[网络/eBPF/02-ebpf-map-types-data-structures.md\|Map 类型]] | 数据结构与存储 | advanced |
| [[网络/eBPF/03-cilium-cni-architecture.md\|Cilium CNI]] | 架构/数据平面 | advanced |
| [[网络/eBPF/04-cilium-network-policy.md\|Cilium 策略]] | L3-L7 网络策略 | intermediate |
| [[网络/eBPF/05-cilium-service-mesh.md\|Cilium Mesh]] | 无 Sidecar Service Mesh | advanced |
| [[网络/eBPF/06-tetragon-runtime-security.md\|Tetragon]] | 运行时安全检测 | advanced |
| [[网络/eBPF/07-hubble-network-observability.md\|Hubble]] | 网络流量可视化 | intermediate |
| [[网络/eBPF/08-bcc-bpftrace-tools.md\|BCC/bpftrace]] | 动态追踪工具 | advanced |
| [[网络/eBPF/09-ebpf-performance-optimization.md\|性能优化]] | eBPF 程序调优 | advanced |
| [[网络/eBPF/10-ebpf-security-applications.md\|安全应用]] | eBPF 安全场景 | advanced |

## Related

- [[网络/网络基础/index.md|网络基础]]
- [[网络/服务网格/index.md|Service Mesh]]
- [[可观测性/工具/index.md|可观测性工具]]
