---
title: LoxiLB [entities]
description: '## 概述'
summary: 'LoxiLB 是一个基于 eBPF 的云原生负载均衡器，专注于为 Kubernetes 提供高性能的 L4 负载均衡服务。它可以作为 Kubernetes 的 [[Service|Service]] LoadBalancer、[[Ingress|Ingress]] 控制器或独立的负载均衡网关运行，利用 eBPF/XDP 技术在内核数据面实现线速转发，'
category: entities
tags:
- k8s
- cncf
- networking
- loxilb
- cilium
- opa
- ingress
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
- LoxiLB 是什么
- 如何 LoxiLB
trigger_keywords:
- LoxiLB
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# LoxiLB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, C

## 概述

LoxiLB 是一个基于 eBPF 的云原生负载均衡器，专注于为 Kubernetes 提供高性能的 L4 负载均衡服务。它可以作为 Kubernetes 的 [[Service|Service]] LoadBalancer、[[Ingress|Ingress]] 控制器或独立的负载均衡网关运行，利用 eBPF/XDP 技术在内核数据面实现线速转发，支持 BGP、ECMP、DSR（Direct Server Return）等高级网络特性，...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **部署模式**: 裸金属集群推荐外部模式（独立 LB 节点），小集群可用 in-cluster 模式
- **BGP 配置**: 与上游路由器建立 BGP 邻居，实现 VIP 的自动广播
- **DSR 模式**: 高流量服务启用 DSR 减少回程带宽消耗
- **健康检查**: 启用端点健康检查，自动剔除问题后端
- **监控**: 监控 eBPF map 的连接数和流量统计

## 架构定位

在 CNCF 生态中，loxilb 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]

## Related

- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[serverless-devs]] — Serverless Devs
- [[sermant]] — Sermant
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- loxilb
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
