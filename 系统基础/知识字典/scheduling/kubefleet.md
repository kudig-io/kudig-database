---
title: KubeFleet 多集群调度
description: KubeFleet 是微软开源的 CNCF Sandbox 项目，提供 Kubernetes 多集群的应用编排和调度，通过 Fleet 概念统一管理大量集群的应...
summary: KubeFleet 是微软开源的 CNCF Sandbox 项目，提供 Kubernetes 多集群的应用编排和调度，通过 Fleet 概念统一管理大量集群的应...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- multi-cluster
- fleet
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeFleet 多集群调度 是什么
- KubeFleet 详解
trigger_keywords:
- KubeFleet 多集群调度
- KubeFleet
- dictionary
prerequisites:
- kubernetes
---



# KubeFleet 多集群调度（KubeFleet）

## 概述

KubeFleet 是微软开源的 CNCF Sandbox 项目，提供 Kubernetes 多集群的应用编排和调度，通过 Fleet 概念统一管理大量集群的应用分发和生命周期。

## 核心概念/原理

- **Fleet 管理**：统一管理数百个 K8s 集群
- **智能调度**：基于集群能力和亲和性选择目标
- **CNCF Sandbox**：微软 Azure Fleet 的开源核心
- **渐进式发布**：支持分批滚动部署

## 关键机制或特性

- MemberCluster CRD 集群注册
- InternalMemberCluster 集群状态
- ClusterResourcePlacement 资源分发
- ClusterSchedulingPolicy 调度策略
- 分批滚动更新（Rolling Update）
- 集群能力感知调度
- Work CRD 资源同步

## 使用场景与最佳实践

- 大规模多集群应用分发
- 边缘集群的集中管理
- 应用的渐进式多集群发布
- 集群资源能力的智能调度
- 全球分布的应用编排

## 参考链接

- https://github.com/Azure/fleet
- https://aka.ms/kubefleet

## Related

- [[系统基础/知识字典/platform-engineering/kubestellar.md|KubeStellar]]
- [[系统基础/知识字典/platform-engineering/karmada.md|Karmada]]
- [[系统基础/知识字典/platform-engineering/open-cluster-management.md|OCM]]
