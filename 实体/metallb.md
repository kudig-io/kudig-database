---
title: MetalLB (entities)
description: '## 概述'
summary: 'MetalLB 是为裸金属 Kubernetes 集群提供的负载均衡器实现。在云环境中，Kubernetes LoadBalancer 类型的 [[Service|Service]] 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2 (ARP/NDP) 或 BGP 协议为 Service 分配和公告外部 IP 地址。'
category: entities
tags:
- k8s
- cncf
- networking
- metallb
- prometheus
- grafana
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MetalLB 是什么
- 如何 MetalLB
trigger_keywords:
- MetalLB
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# MetalLB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

MetalLB 是为裸金属 Kubernetes 集群提供的负载均衡器实现。在云环境中，Kubernetes LoadBalancer 类型的 [[Service|Service]] 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2 (ARP/NDP) 或 BGP 协议为 Service 分配和公告外部 IP 地址。

## 核心能力

- **Layer 2 模式**: 使用 ARP (IPv4) 或 NDP (IPv6) 响应本地网络请求
- **BGP 模式**: 与网络路由器建立 BGP 会话公告服务 IP
- **IP 地址池**: 灵活配置可分配的 IP 地址范围
- **自动故障转移**: Leader 选举确保 L2 模式高可用
- **双栈支持**: 同时支持 IPv4 和 IPv6
- **CRD 配置**: 使用 Kubernetes 原生资源配置

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **网络规划**: 确保 IP 地址池与现有网络不冲突
- **L2 限制**: Layer 2 模式下单节点承载所有流量，考虑带宽瓶颈
- **BGP 优先**: 生产环境推荐 BGP 模式实现真正负载均衡
- **故障排查**: 使用 `speaker` Pod 日志排查网络公告问题
- **IP 预留**: 为关键服务预分配固定 IP
- **监控告警**: 配置 IP 池耗尽告警

## 架构定位

在 CNCF 生态中，metallb 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[athenz]] — Athenz
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- metallb
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
