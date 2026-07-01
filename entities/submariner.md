---
title: Submariner (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- submariner
- gateway
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Submariner 是什么
- 如何 Submariner
trigger_keywords:
- Submariner
prerequisites:
- kubectl-basics
---



# Submariner

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Submariner 实现 Kubernetes 多集群之间的 Pod 和 [[Service|Service]] 网络直连。它在集群之间建立加密隧道 (IPsec/WireGuard)，允许跨集群的 Pod 直接通信和 Service 发现，解决了多集群环境下的网络连通性问题。

## 核心能力

- **跨集群 Pod 网络**: Pod 到 Pod 直接通信
- **跨集群 Service 发现**: 使用 ServiceImport/ServiceExport
- **加密隧道**: IPsec 或 WireGuard 加密
- **Globalnet**: 处理重叠 CIDR 的情况
- **Gateway 选举**: 自动选举网关节点
- **Lighthouse DNS**: 跨集群 DNS 解析

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **网关节点**: 为网关节点分配足够带宽
- **CIDR 规划**: 提前规划避免 CIDR 重叠
- **WireGuard**: 推荐使用 WireGuard 替代 IPsec
- **监控**: 监控隧道状态和延迟
- **高可用**: 配置多个网关节点

## 架构定位

在 CNCF 生态中，submariner 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[pod-lifecycle]]

## Related

- [[hwameistor]] — HwameiStor
- [[dragonfly]] — Dragonfly
- [[aeraki-mesh]] — Aeraki Mesh
- [[atlantis]] — Atlantis
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- submariner
- [[skills/ts-cloud-provider.md|云服务商集成排查]] — Cross-reference
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
