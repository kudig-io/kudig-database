---
title: Kmesh (entities)
description: '## 概述'
summary: 'Kmesh 是一个基于 eBPF 和可编程内核的无 Sidecar 服务网格，在内核空间实现流量治理能力。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。'
category: entities
tags:
- k8s
- cncf
- networking
- kmesh
- istio
- envoy
- cilium
- crd
- operator
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kmesh 是什么
- 如何 Kmesh
trigger_keywords:
- Kmesh
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kmesh

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, C

## 概述

Kmesh 是一个基于 eBPF 和可编程内核的无 Sidecar 服务网格，在内核空间实现流量治理能力。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **内核版本**: 确保节点内核版本 >= 5.10，推荐 5.15+ 获得最佳 eBPF 支持
- **渐进迁移**: 从非关键服务开始启用 Kmesh，验证功能后逐步扩大范围
- **监控**: 利用 Kmesh 导出的 eBPF 指标监控流量治理效果
- **混合模式**: 可与 Istio Sidecar 在同一集群中共存，按命名空间选择模式
- **安全策略**: 配合 Istio 的 mTLS 和授权策略使用

## 架构定位

在 CNCF 生态中，kmesh 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]
- [[cilium]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[opengemini]] — openGemini
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kmesh
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
