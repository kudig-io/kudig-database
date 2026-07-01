---
title: MetalLB
description: MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如
  on-pre...
summary: MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如
  on-pre...
category: dictionary
tags:
- k8s
- glossary
- metallb
- loadbalancer
- networking
- bare-metal
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MetalLB 是什么
- MetalLB 详解
trigger_keywords:
- MetalLB
- dictionary
prerequisites:
- kubectl-basics
---



# MetalLB

> **英文名**: MetalLB

## 概述

MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如 on-premises）提供 LoadBalancer 类型的 Service 支持，是本地 K8s 集群的必备组件。

## 核心概念/原理

### 工作模式

| 模式 | 说明 | 适用场景 |
|------|------|----------|
| Layer 2 | ARP/NDP 应答 | 简单场景，单节点故障转移 |
| BGP | 与路由器对等 | 大规模，多路径，快速故障转移 |

### 工作原理

```
External Client → LoadBalancer IP → [MetalLB ARP/BGP] → Node → kube-proxy → Pod
```

## 关键机制或特性

- **IP Address Pool**：定义可分配的 IP 地址范围。
- **L2 Advertisement**：通过 ARP 通告 VIP。
- **BGP Advertisement**：通过 BGP 协议通告路由。
- **speaker** DaemonSet：每节点运行，负责 IP 通告。
- **controller**：分配 IP 和管理配置。

## 使用场景与最佳实践

- 裸金属集群必须安装 MetalLB 支持 LoadBalancer Service。
- 简单场景使用 Layer 2 模式。
- 大规模生产环境使用 BGP 模式配合 ToR 交换机。
- 为不同 Service 分配不同的 IP Pool。
- 监控 MetalLB 的 BGP 会话状态和 IP 分配情况。

## 参考链接

- [MetalLB Official](https://metallb.universe.tf/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/loadbalancer.md|LoadBalancer]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/nodeport.md|NodePort]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kube-proxy.md|Kube-proxy]]
