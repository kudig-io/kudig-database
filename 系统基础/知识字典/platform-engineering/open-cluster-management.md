---
title: Open Cluster Management
description: Open Cluster Management（OCM）是 Red Hat 主导的 CNCF Sandbox 项目，提供 Kubernetes
  多集群的管理框架...
summary: Open Cluster Management（OCM）是 Red Hat 主导的 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理框架...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- multi-cluster
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Cluster Management 是什么
- OCM 详解
trigger_keywords:
- Open Cluster Management
- OCM
- dictionary
prerequisites:
- kubernetes
---



# Open Cluster Management（OCM）

## 概述

Open Cluster Management（OCM）是 Red Hat 主导的 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理框架，包括集群注册、策略下发、应用部署和可观测性聚合。

## 核心概念/原理

- **Hub-Spoke 架构**：中心 Hub 集群管理多个 Spoke（被管理）集群
- **策略引擎**：通过 Policy 框架实现跨集群配置合规管理
- **应用生命周期**：Placement + Subscription 实现应用分发
- **CNCF Sandbox**：Red Hat ACM 的开源核心

## 关键机制或特性

- Klusterlet Agent 部署在被管理集群
- ManifestWork 向被管理集群下发资源
- Placement API 选择目标集群
- Policy 框架（配置合规、安全策略、Operator 部署）
- Search API 跨集群资源搜索
- Application 模型管理多集群应用

## 使用场景与最佳实践

- 企业级多集群运维管理
- 跨集群安全策略和合规检查
- 集中式应用分发和生命周期管理
- 多集群可观测性聚合
- 混合云集群的统一控制面

## 参考链接

- https://open-cluster-management.io/
- https://github.com/open-cluster-management-io/ocm

## Related

- [[系统基础/知识字典/platform-engineering/karmada.md|Karmada]]
- [[系统基础/知识字典/platform-engineering/rancher.md|Rancher]]
- [[系统基础/知识字典/security/opa.md|OPA]]
