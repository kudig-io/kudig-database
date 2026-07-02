---
title: VXLAN
description: VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernete...
summary: VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernete...
category: dictionary
tags:
- k8s
- glossary
- networking
- vxlan
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- VXLAN 是什么
- VXLAN 详解
trigger_keywords:
- VXLAN
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# VXLAN

> **英文名**: VXLAN

## 概述

VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernetes CNI 插件广泛使用 VXLAN 实现 Pod 间的跨节点通信。

## 核心概念/原理

### 核心概念

- **VTEP（VXLAN Tunnel Endpoint）**：封装和解封装的端点（通常是节点上的虚拟网络设备）。
- **VNI（VXLAN Network Identifier）**：24 位的网络标识，支持最多 1600 万个虚拟网络。
- **封装方式**：原始 Pod 数据包 → 以太网帧 → UDP（端口 4789）→ 外层 IP 包。

### 在 Kubernetes 中的应用

- **Flannel**：VXLAN 后端是最常用的模式。
- **Calico**：支持 VXLAN 封装模式。
- **Cilium**：支持 VXLAN 隧道模式。

## 关键机制或特性

- VXLAN 增加了约 50 字节的头部开销。
- 相比 IPIP 封装，VXLAN 支持跨三层的虚拟网络。
- 硬件卸载（checksum offload）可以提升 VXLAN 性能。

## 使用场景与最佳实践

- 大规模集群中 VXLAN 的封装开销需要考虑。
- 高性能场景考虑使用 eBPF（Cilium）替代 VXLAN。
- 确保 UDP 4789 端口在节点间可达。

## 参考链接

- [VXLAN - Official Documentation](https://datatracker.ietf.org/doc/html/rfc7348)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/topic-dictionary/networking/clusterip.md|Clusterip]]
- [[domain-17-system-foundation/topic-dictionary/networking/nodeport.md|Nodeport]]
- [[domain-17-system-foundation/topic-dictionary/networking/loadbalancer.md|Loadbalancer]]


<!-- risk-assessed -->
