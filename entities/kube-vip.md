---
title: kube-vip (entities)
description: '## 概述'
summary: 'kube-vip 为 Kubernetes 集群提供虚拟 IP (VIP) 和负载均衡功能。它可以作为控制平面的高可用解决方案，提供浮动 VIP 确保 API Server 始终可访问。同时也可以作为 LoadBalancer 类型 [[Service|Service]] 的实现，为裸金属环境提供服务负载均衡。'
category: entities
tags:
- k8s
- cncf
- networking
- kube-vip
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-vip 是什么
- 如何 kube-vip
trigger_keywords:
- kube-vip
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-vip

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

kube-vip 为 Kubernetes 集群提供虚拟 IP (VIP) 和负载均衡功能。它可以作为控制平面的高可用解决方案，提供浮动 VIP 确保 API Server 始终可访问。同时也可以作为 LoadBalancer 类型 [[Service|Service]] 的实现，为裸金属环境提供服务负载均衡。

## 核心能力

- **控制平面 HA**: 为 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] Server 提供 VIP
- **Service LoadBalancer**: 裸金属 LoadBalancer 实现
- **ARP/BGP**: 支持 Layer 2 (ARP) 和 Layer 3 (BGP) 模式
- **Leader 选举**: 基于 Raft 或 Kubernetes Lease 的选举
- **轻量级**: 单一二进制，无外部依赖
- **IPv4/IPv6**: 双栈支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **接口选择**: 指定正确的网络接口
- **IP 规划**: 确保 VIP 不与 DHCP 范围冲突
- **HA 模式**: 控制平面至少 3 个节点
- **Lease 调优**: 根据网络质量调整选举参数
- **BGP 场景**: 大规模集群推荐 BGP 模式
- **监控**: 监控 VIP 漂移和选举事件

## 架构定位

在 CNCF 生态中，kube-vip 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[tuf]] — TUF
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-vip
- [[entities/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]] — Cross-reference
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
