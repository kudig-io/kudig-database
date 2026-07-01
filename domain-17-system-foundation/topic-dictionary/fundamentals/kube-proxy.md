---
title: kube-proxy
description: kube-proxy 是运行在每个节点上的网络代理，负责维护节点上的网络规则以实现 Service 的负载均衡功能。它是 Kubernetes
  Service ...
summary: kube-proxy 是运行在每个节点上的网络代理，负责维护节点上的网络规则以实现 Service 的负载均衡功能。它是 Kubernetes Service
  ...
category: dictionary
tags:
- k8s
- glossary
- kube-proxy
- networking
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-proxy 是什么
- kube-proxy 详解
trigger_keywords:
- kube-proxy
- dictionary
prerequisites:
- kubectl-basics
---



# kube-proxy

> **英文名**: kube-proxy

## 概述

kube-proxy 是运行在每个节点上的网络代理，负责维护节点上的网络规则以实现 Service 的负载均衡功能。它是 Kubernetes Service 抽象的底层实现。

## 核心概念/原理

### 代理模式

kube-proxy 支持多种工作模式：

- **iptables 模式**（默认）：使用 iptables 规则实现流量转发，性能好但规则数随 Service 增长线性增加。
- **IPVS 模式**：使用 Linux IPVS（IP Virtual Server）内核模块，规则数 O(1) 复杂度，适合大规模集群。
- **nftables 模式**（v1.31+ Alpha）：使用 nftables 替代 iptables，提供更好的性能和可维护性。

### 工作原理

kube-proxy 监听 Service 和 Endpoints/EndpointSlice 的变化，自动更新节点上的转发规则。

## 关键机制或特性

- Service 的 ClusterIP 流量通过 kube-proxy 分发的规则转发到后端 Pod。
- IPVS 模式需要节点安装 `ipvsadm` 等工具并加载相应内核模块。
- kube-proxy 支持会话保持（SessionAffinity）。

## 使用场景与最佳实践

- 大规模集群（>1000 Service）建议使用 IPVS 模式。
- 监控 kube-proxy 的同步延迟和错误。
- 使用 `kube-proxy --proxy-mode=ipvs` 切换到 IPVS 模式时需确保内核模块就绪。
- 考虑使用 eBPF 替代方案（如 Cilium）获得更好的性能。

## 参考链接

- [kube-proxy - Official Documentation](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-proxy/)

## Related

[[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
