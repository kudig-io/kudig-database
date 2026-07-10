---
title: 污点和容忍
description: Taints（污点）和 Tolerations（容忍）是 Kubernetes 的调度约束机制，节点通过 Taint 排斥不匹配的 Pod，Pod
  通过 Tol...
summary: Taints（污点）和 Tolerations（容忍）是 Kubernetes 的调度约束机制，节点通过 Taint 排斥不匹配的 Pod，Pod
  通过 Tol...
category: dictionary
tags:
- k8s
- glossary
- configuration
- scheduling
- node
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 污点和容忍 是什么
- Taints and Tolerations 详解
trigger_keywords:
- 污点和容忍
- Taints and Tolerations
- dictionary
prerequisites:
- kubernetes
---



# 污点和容忍（Taints and Tolerations）

## 概述

Taints（污点）和 Tolerations（容忍）是 Kubernetes 的调度约束机制，节点通过 Taint 排斥不匹配的 Pod，Pod 通过 Toleration 声明接受特定污点，实现节点级的工作负载隔离。

## 核心概念/原理

- **节点排斥**：Taint 让节点拒绝不匹配的 Pod
- **Pod 容忍**：Toleration 让 Pod 接受特定污点
- **三效果**：NoSchedule/PreferNoSchedule/NoExecute
- **系统级**：Master/控制节点的标准隔离方案

## 关键机制或特性

- `taint`: key=value:effect
- NoSchedule：新 Pod 不调度到此节点
- PreferNoSchedule：尽量不调度（软限制）
- NoExecute：驱逐已运行的不容忍 Pod
- tolerationSeconds：NoExecute 的容忍时间
- 系统污点：node.kubernetes.io/not-ready/unreachable
- DaemonSet 自动容忍常见污点

## 使用场景与最佳实践

- 专用节点（GPU/SSD/高配）的工作负载隔离
- Master/控制节点的保护性污点
- 节点维护时的 Pod 驱逐
- 多租户的节点隔离
- 最佳实践：专用节点用 NoSchedule、故障用 NoExecute+tolerationSeconds、避免滥用

## 参考链接

- https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/
- https://kubernetes.io/docs/reference/labels-annotations-taints/

## Related

- [[系统基础/知识字典/scheduling/affinity.md|Node Affinity]]
- [[系统基础/知识字典/scheduling/topology-spread-constraints.md|Topology Spread]]
- [[系统基础/知识字典/fundamentals/cluster.md|Cluster]]
