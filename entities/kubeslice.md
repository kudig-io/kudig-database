---
title: KubeSlice (entities)
description: '## 概述'
summary: 'KubeSlice 是一个多集群网络平台，通过创建逻辑 Slice（网络切片）覆盖层，在多个 Kubernetes 集群之间建立扁平的、安全的网络连接。每个 Slice 提供独立的网络命名空间、QoS 策略和安全隔离，使跨集群的应用能够像在同一集群内一样通信，同时保持网络隔离和带宽保障。'
category: entities
tags:
- k8s
- cncf
- networking
- kubeslice
- istio
- cilium
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
- KubeSlice 是什么
- 如何 KubeSlice
trigger_keywords:
- KubeSlice
prerequisites:
- kubectl-basics
- service-mesh-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeSlice

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

KubeSlice 是一个多集群网络平台，通过创建逻辑 Slice（网络切片）覆盖层，在多个 Kubernetes 集群之间建立扁平的、安全的网络连接。每个 Slice 提供独立的网络命名空间、QoS 策略和安全隔离，使跨集群的应用能够像在同一集群内一样通信，同时保持网络隔离和带宽保障。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Slice 规划**: 按业务域划分 Slice，每个 Slice 服务于一组关联的微服务
- **QoS 配置**: 为关键业务 Slice 配置带宽保障，避免非关键流量抢占
- **网络隔离**: 启用 namespaceIsolation 确保 Slice 间的安全隔离
- **网关选择**: 低延迟场景使用 WireGuard，兼容性优先使用 OpenVPN
- **监控**: 监控各 Slice 的带宽利用率、延迟和网关隧道状态

## 架构定位

在 CNCF 生态中，kubeslice 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]
- [[cilium]]
- [[deployment]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[kubeclipper]] — KubeClipper
- [[runme-notebooks]] — Runme
- [[operator-framework]] — Operator Framework
- [[clusternet]] — Clusternet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeslice
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
