---
title: Kubernetes 集群
description: Kubernetes 集群是由一组节点（Node）组成的计算资源池，包含控制平面（Control Plane）和工作节点（Worker Node），提供容器编排...
summary: Kubernetes 集群是由一组节点（Node）组成的计算资源池，包含控制平面（Control Plane）和工作节点（Worker Node），提供容器编排...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- cluster
- architecture
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 集群 是什么
- Cluster 详解
trigger_keywords:
- Kubernetes 集群
- Cluster
- dictionary
prerequisites:
- kubernetes
---



# Kubernetes 集群（Cluster）

## 概述

Kubernetes 集群是由一组节点（Node）组成的计算资源池，包含控制平面（Control Plane）和工作节点（Worker Node），提供容器编排、调度和生命周期管理能力。

## 核心概念/原理

- **控制平面**：API Server/Scheduler/Controller Manager/etcd
- **工作节点**：kubelet/kube-proxy/Container Runtime
- **高可用**：多 Master + etcd 集群
- **可扩展**：CRD/Operator/Webhook 扩展点

## 关键机制或特性

- 集群 = Control Plane + Worker Nodes
- API Server 是唯一入口（所有操作经此）
- etcd 存储集群状态（Raft 共识）
- Scheduler 决定 Pod 放置
- kubelet 管理节点上的 Pod
- kube-proxy 维护网络规则
- 集群联邦（Federation）管理多集群

## 使用场景与最佳实践

- 生产环境高可用部署（3+ Master）
- 多租户集群隔离
- 集群升级和证书轮转
- 集群网络安全加固
- 多区域/多可用区部署
- 最佳实践：托管 K8s（EKS/AKS/GKE）降低运维负担

## 参考链接

- https://kubernetes.io/docs/concepts/cluster-administration/
- https://kubernetes.io/docs/setup/

## Related

- [[domain-17-system-foundation/知识字典/fundamentals/kubernetes.md|Kubernetes]]
- [[domain-17-system-foundation/知识字典/fundamentals/namespace.md|Namespace]]
- [[domain-17-system-foundation/知识字典/fundamentals/etcd.md|etcd]]
