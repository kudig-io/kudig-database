---
title: Antrea 网络方案
description: Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy...
summary: Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy...
category: dictionary
tags:
- k8s
- glossary
- networking
- cni
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
- Antrea 网络方案 是什么
- Antrea 详解
trigger_keywords:
- Antrea 网络方案
- Antrea
- dictionary
prerequisites:
- kubernetes
---



# Antrea 网络方案（Antrea）

## 概述

Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy、流量可视化、多集群网络等企业级功能，是 Calico/Cilium 之外的另一主流 CNI 选择。

## 核心概念/原理

- **OVS 数据面**：基于 Open vSwitch 的高性能转发引擎
- **完整 NetworkPolicy**：支持 K8s NetworkPolicy + Antrea 扩展策略（FQDN 策略、NodeNetworkPolicy）
- **流量可视化**：内置 Flow Exporter 和 ClickHouse 集成
- **多集群支持**：Antrea Multi-cluster 实现跨集群网络互通

## 关键机制或特性

- OVS 流表驱动的转发规则管理
- 支持 WireGuard 加密隧道
- Egress / ExternalIP 管理
- Traceflow 端到端连通性诊断
- 与 Theia 可视化平台集成
- 支持 Antrea Proxy（kube-proxy 替代）

## 使用场景与最佳实践

- 企业级 K8s 网络方案选型
- 需要高级 NetworkPolicy（FQDN、Node 级别）
- 网络流量审计与可视化需求
- 多集群网络互联场景

## 参考链接

- https://antrea.io/
- https://github.com/antrea-io/antrea

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/cilium.md|Cilium]]
- [[domain-17-system-foundation/topic-dictionary/networking/cni.md|CNI]]
- [[domain-17-system-foundation/topic-dictionary/networking/networkpolicy.md|NetworkPolicy]]
