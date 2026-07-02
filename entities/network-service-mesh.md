---
title: Network Service Mesh (NSM)
description: '## 概述'
summary: 'Network Service Mesh (NSM) 是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务，例如安全隧道、VPN、防火墙等网络功能的动态连接。'
category: entities
tags:
- k8s
- cncf
- networking
- network-service-mesh
- prometheus
- grafana
- istio
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
- Network Service Mesh (NSM) 是什么
- 如何 Network Service Mesh (NSM)
trigger_keywords:
- Network
- Service
- Mesh
- NSM
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---



# [[entities/network-service-mesh.md|Network Service Mesh]]rvice]]Service Mesh）|Service Mesh]] (NSM)

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Network Service Mesh (NSM) 是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务，例如安全隧道、VPN、防火墙等网络功能的动态连接。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **安全优先**: 始终启用 SPIRE 进行工作负载身份认证和 mTLS 加密
- **数据平面选择**: 低延迟/高吞吐场景使用 VPP，常规场景使用 Kernel
- **资源规划**: 每个节点的 NSMgr 需要预留足够的 CPU 和内存
- **网络规划**: 提前规划 NSM 使用的 IP 地址段，避免与集群网络冲突
- **跨集群部署**: 使用 DNS 代理实现跨集群服务发现，确保集群间网络可达
- **故障恢复**: NSM 内置连接自愈机制，确保 NSE 具有适当的健康检查

## 架构定位

在 CNCF 生态中，network-service-mesh 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[deployment]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[chaosblade]] — ChaosBlade
- [[istio]] — Istio
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[spire]] — SPIRE

- network-service-mesh
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
