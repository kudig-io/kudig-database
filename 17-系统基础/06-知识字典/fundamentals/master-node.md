---
title: 主节点
description: Master Node（主节点）是运行 Kubernetes 控制平面组件的节点。在现代 Kubernetes 术语中，更推荐使用 Control
  Plane ...
summary: Master Node（主节点）是运行 Kubernetes 控制平面组件的节点。在现代 Kubernetes 术语中，更推荐使用 Control
  Plane ...
category: dictionary
tags:
- k8s
- glossary
- control-plane
- node
tier: peripheral
created: '2026-06-24'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[17-系统基础/06-知识字典/workloads/pod.md|Pod]]
- [[17-系统基础/06-知识字典/fundamentals/container.md|Container]]
- [[17-系统基础/06-知识字典/fundamentals/namespace.md|Namespace]]
- [[17-系统基础/06-知识字典/fundamentals/cluster.md|Cluster]]
- [[17-系统基础/06-知识字典/fundamentals/control-plane.md|Control Plane]]


<!-- risk-assessed -->
