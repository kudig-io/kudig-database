---
title: Pod
description: Pod 是 Kubernetes 的最小调度单元和计算单元。一个 Pod 封装了一个或多个紧密相关的容器，共享网络和存储资源，并作为一个整体被调度和管理。...
summary: Pod 是 Kubernetes 的最小调度单元和计算单元。一个 Pod 封装了一个或多个紧密相关的容器，共享网络和存储资源，并作为一个整体被调度和管理。...
category: dictionary
tags:
- k8s
- glossary
- pod
- workload
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 是什么
- Pod 详解
trigger_keywords:
- Pod
- dictionary
prerequisites:
- kubectl-basics
---



# Pod

> **英文名**: Pod

## 概述

Pod 是 Kubernetes 的最小调度单元和计算单元。一个 Pod 封装了一个或多个紧密相关的容器，共享网络和存储资源，并作为一个整体被调度和管理。

## 核心概念/原理

### 核心特性

- **共享网络**：Pod 内的容器共享同一个网络命名空间（相同的 IP 和端口空间）。
- **共享存储**：Pod 可以挂载 Volume，容器间共享数据。
- **生命周期**：Pod 经历 Pending → Running → Succeeded/Failed 的生命周期。

### Pod 类型

- **单容器 Pod**：最常见的模式，一个 Pod 运行一个容器。
- **多容器 Pod（Sidecar 模式）**：主容器 + 辅助容器（日志收集、服务网格代理等）。
- **Init Container**：在主容器启动前运行的初始化容器。
- **Static Pod**：由 kubelet 直接管理，不经过 API Server。

## 关键机制或特性

- Pod 的 `restartPolicy` 控制容器重启策略（Always/OnFailure/Never）。
- Pod 的 QoS 类别由资源 Request 和 Limit 决定。
- Pod 可以通过 OwnerReference 关联到上层控制器（Deployment/ReplicaSet 等）。
- Pod Disruption Budget（PDB）限制同时被驱逐的 Pod 数量。

## 使用场景与最佳实践

- 尽量保持一个 Pod 运行一个主容器（single container per Pod 原则）。
- 为容器设置资源 Request 和 Limit。
- 配置 Liveness 和 Readiness 探针确保健康检查。
- 使用 `terminationGracePeriodSeconds` 实现优雅关闭。

## 参考链接

- [Pod - Official Documentation](https://kubernetes.io/docs/concepts/workloads/pods/)

## Related

[[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-components.md|Kubernetes 组件]]
