---
title: Antrea [entities]
description: '## 概述'
summary: 'Antrea 是基于 Open vSwitch (OVS) 构建的 Kubernetes 网络解决方案，为 Pod 网络提供高性能数据平面。它实现了 Kubernetes [[NetworkPolicy|NetworkPolicy]] API，并扩展支持更细粒度的流量控制，包括 ClusterNetworkPolicy、Egress 和流量可观测性功能。'
category: entities
tags:
- k8s
- cncf
- networking
- antrea
- prometheus
- grafana
- gateway
- networkpolicy
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Antrea 是什么
- 如何 Antrea
trigger_keywords:
- Antrea
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Antrea

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Antrea 是基于 Open vSwitch (OVS) 构建的 Kubernetes 网络解决方案，为 Pod 网络提供高性能数据平面。它实现了 Kubernetes [[NetworkPolicy|NetworkPolicy]] API，并扩展支持更细粒度的流量控制，包括 ClusterNetworkPolicy、Egress 和流量可观测性功能。

## 核心能力

- **高性能网络**: 基于 OVS 的优化数据路径
- **NetworkPolicy**: 完整支持 K8s NetworkPolicy + 扩展策略
- **多集群支持**: 跨集群 Pod 网络互通
- **流量可观测性**: Flow Exporter、IPFIX、Packet Tracing
- **Egress 网关**: 集中管理出站流量
- **二层网络**: 支持 VLAN、Trunk 网络

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **隧道选择**: 一般使用 Geneve，性能敏感场景考虑 noEncap
- **策略分层**: 使用 Tier 组织策略优先级
- **流量加密**: 跨区域流量启用 IPsec 或 WireGuard
- **可观测性**: 生产环境部署 Flow Aggregator
- **Traceflow**: 使用 Traceflow 调试网络问题
- **多集群**: 统一 Pod CIDR 规划避免冲突

## 架构定位

在 CNCF 生态中，antrea 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[kgateway]] — kgateway
- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- antrea
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
