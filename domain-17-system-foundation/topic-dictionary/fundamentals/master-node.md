---
title: 主节点
description: 'Master Node（主节点）是运行 Kubernetes 控制平面组件的节点。在现代 Kubernetes 术语中，更推荐使用 Control Plane ...'
category: dictionary
tags:
- k8s
- glossary
- control-plane
- node
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 主节点 是什么
- Master Node / Control Plane Node 详解
trigger_keywords:
- 主节点
- Master Node / Control Plane Node
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 主节点

> **英文名**: Master Node / Control Plane Node

## 概述

Master Node（主节点）是运行 Kubernetes 控制平面组件的节点。在现代 Kubernetes 术语中，更推荐使用 Control Plane Node 来称呼。主节点负责集群的管理、调度和状态维护。

## 核心概念/原理

### 运行的组件
- **kube-apiserver**：集群 API 入口。
- **etcd**：集群状态存储。
- **kube-scheduler**：Pod 调度决策。
- **kube-controller-manager**：控制器运行。
- **cloud-controller-manager**（可选）：云平台集成。

### 高可用部署
生产环境通常部署 3 或 5 个控制平面节点：
- 3 节点：容忍 1 个节点故障。
- 5 节点：容忍 2 个节点故障。
- 通过 `node-role.kubernetes.io/control-plane` 标签和 NoSchedule 污点隔离。

## 关键机制或特性

- 从 K8s v1.20 起，官方弃用 `master` 术语，改用 `control-plane`。
- kubeadm 初始化的控制平面节点自动添加污点 `node-role.kubernetes.io/control-plane:NoSchedule`。
- 控制平面节点可以是专用的（dedicated）或与 Worker Node 共享（不推荐生产）。

## 使用场景与最佳实践

- 生产环境使用专用的控制平面节点。
- 控制平面节点应部署在不同的故障域（可用区）。
- 监控控制平面节点的资源使用和 etcd 健康状态。

## 参考链接

- [Master Node / Control Plane Node - Official Documentation](https://kubernetes.io/docs/concepts/architecture/)

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/container.md|Container]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace.md|Namespace]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster.md|Cluster]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/control-plane.md|Control Plane]]
