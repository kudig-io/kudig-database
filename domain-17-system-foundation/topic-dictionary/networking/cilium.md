---
title: Cilium
description: Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、...
summary: Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、...
category: dictionary
tags:
- k8s
- glossary
- cilium
- cni
- ebpf
- networkpolicy
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium 是什么
- Cilium 详解
trigger_keywords:
- Cilium
- dictionary
prerequisites:
- kubectl-basics
---



# Cilium

> **英文名**: Cilium

## 概述

Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、细粒度的安全策略和深度可观测性，已成为云原生网络的事实标准之一。

## 核心概念/原理

### 核心架构

- **eBPF 数据平面**：在内核态处理网络包，替代 kube-proxy 的 iptables。
- **Cilium Agent**：每节点 DaemonSet，管理策略和配置。
- **Hubble**：内置的网络可观测性组件，提供流量可视化。
- **Cilium Operator**：集群级管理组件（IPAM、身份管理）。

### 与 iptables 对比

| 特性 | iptables | Cilium (eBPF) |
|------|----------|---------------|
| 规则处理 | O(n) 线性扫描 | O(1) 哈希查找 |
| 策略粒度 | L3/L4 | L3-L7（含 HTTP/gRPC） |
| 性能 | 规则多时性能下降 | 恒定性能 |

## 关键机制或特性

- 完全替代 kube-proxy，使用 eBPF 实现 Service 负载均衡。
- 支持 FQDN Policy（基于域名的网络策略）。
- 支持 Cluster Mesh 实现多集群网络互通。
- Gateway API 原生支持。
- Tetragon 提供运行时安全检测和进程级可观测性。

## 使用场景与最佳实践

- 新集群优先选择 Cilium 作为 CNI。
- 启用 Cilium 的 kube-proxy 替代模式提升 Service 性能。
- 使用 Hubble 进行网络故障排查和流量分析。
- 配合 CiliumNetworkPolicy 实现 L7 层安全策略。
- 使用 Cilium CLI 进行安装和诊断。

## 参考链接

- [Cilium Official Documentation](https://docs.cilium.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/cni.md|CNI]]
- [[domain-17-system-foundation/topic-dictionary/networking/networkpolicy.md|NetworkPolicy]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kube-proxy.md|Kube-proxy]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
