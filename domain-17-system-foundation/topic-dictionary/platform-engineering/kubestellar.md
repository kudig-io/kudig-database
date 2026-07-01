---
title: KubeStellar 多集群分发
description: 'KubeStellar 是 IBM 开源的 CNCF Sandbox 项目，提供基于 Kubernetes 原生的多集群工作负载分发和同步，利用 KCP（Kub...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- multi-cluster
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeStellar 多集群分发 是什么
- KubeStellar 详解
trigger_keywords:
- KubeStellar 多集群分发
- KubeStellar
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# KubeStellar 多集群分发（KubeStellar）

## 概述

KubeStellar 是 IBM 开源的 CNCF Sandbox 项目，提供基于 Kubernetes 原生的多集群工作负载分发和同步，利用 KCP（Kubernetes Control Plane）实现跨集群的声明式资源管理。

## 核心概念/原理

- **KCP 架构**：基于 KCP 的多集群控制面
- **透明分发**：应用无需修改即可分发到多集群
- **CNCF Sandbox**：IBM Research 主导
- **K8s 原生**：不引入新的抽象层

## 关键机制或特性

- BindingPolicy 定义资源分发策略
- Location 描述目标集群特征
- Inventory 集群注册和发现
- 基于标签的集群选择
- 资源状态聚合
- 与 Karmada/OCM 互补的多集群方案

## 使用场景与最佳实践

- 大规模多集群的应用分发
- 边缘集群的集中管理
- 多区域部署的透明管理
- 企业多集群治理
- 集群生命周期管理

## 参考链接

- https://kubestellar.io/
- https://github.com/kubestellar/kubestellar

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/karmada.md|Karmada]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/open-cluster-management.md|OCM]]
- [[domain-17-system-foundation/topic-dictionary/networking/clusternet.md|Clusternet]]
