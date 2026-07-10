---
title: LoxiLB eBPF 负载均衡
description: LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB
  + ku...
summary: LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB
  + ku...
category: dictionary
tags:
- k8s
- glossary
- networking
- load-balancer
- ebpf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LoxiLB eBPF 负载均衡 是什么
- LoxiLB 详解
trigger_keywords:
- LoxiLB eBPF 负载均衡
- LoxiLB
- dictionary
prerequisites:
- kubernetes
---



# LoxiLB eBPF 负载均衡（LoxiLB）

## 概述

LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB + kube-proxy + external LB 的组合。

## 核心概念/原理

- **eBPF 驱动**：使用 eBPF/XDP 实现高性能数据面
- **多模式**：L4/L7 负载均衡、NAT、FW、Egress
- **K8s 原生**：Operator 模式部署，自动感知 Service
- **轻量级**：单进程，资源占用极低

## 关键机制或特性

- Service Type LoadBalancer 自动分配
- kube-proxy 替代（eBPF 模式）
- L4/L7 负载均衡（IPVS 替代）
- 多集群负载均衡
- SCTP 支持（5G/Telco 场景）
- 健康检查和故障转移
- Prometheus 指标导出

## 使用场景与最佳实践

- 裸金属/边缘环境的 LoadBalancer 实现
- MetalLB + kube-proxy 的统一替代
- 5G/Telco 的 SCTP 负载均衡
- 需要 eBPF 高性能的网络方案
- 轻量级外部负载均衡

## 参考链接

- https://loxilb.io/
- https://github.com/loxilb-io/loxilb

## Related

- [[系统基础/知识字典/networking/metallb.md|MetalLB]]
- [[系统基础/知识字典/networking/cilium.md|Cilium]]
- [[系统基础/知识字典/networking/kube-vip.md|kube-vip]]
