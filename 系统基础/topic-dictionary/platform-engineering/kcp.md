---
title: KCP 多租户控制面
description: KCP（Kubernetes-like Control Plane）是 Red Hat 开源的 CNCF Sandbox 项目，提供 Kubernetes
  兼容...
summary: KCP（Kubernetes-like Control Plane）是 Red Hat 开源的 CNCF Sandbox 项目，提供 Kubernetes
  兼容...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- multi-tenancy
- api
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KCP 多租户控制面 是什么
- KCP 详解
trigger_keywords:
- KCP 多租户控制面
- KCP
- dictionary
prerequisites:
- kubernetes
---



# KCP 多租户控制面（KCP）

## 概述

KCP（Kubernetes-like Control Plane）是 Red Hat 开源的 CNCF Sandbox 项目，提供 Kubernetes 兼容的 API 控制面，但不运行容器，用于构建多租户平台和管理跨集群资源。

## 核心概念/原理

- **K8s API 兼容**：提供标准 K8s API 但不运行 Pod
- **多租户**：Workspace 模型实现层次化多租户
- **CNCF Sandbox**：Red Hat 主导
- **元控制面**：管理其他 K8s 集群的控制面

## 关键机制或特性

- Workspace 层次化命名空间
- APIBinding/APIExport 跨 Workspace 资源共享
- Syncer 将资源同步到实际 K8s 集群
- 多集群资源视图
- 与 KubeStellar/OCM 集成
- Placement API 资源放置

## 使用场景与最佳实践

- SaaS 平台的多租户控制面
- 内部开发者平台的 API 层
- 多集群资源编排的元控制面
- K8s API 的定制化扩展
- 服务目录和自助服务平台

## 参考链接

- https://kcp.io/
- https://github.com/kcp-dev/kcp

## Related

- [[系统基础/topic-dictionary/platform-engineering/kubestellar.md|KubeStellar]]
- [[系统基础/topic-dictionary/security/capsule.md|Capsule]]
- [[系统基础/topic-dictionary/platform-engineering/backstage.md|Backstage]]
