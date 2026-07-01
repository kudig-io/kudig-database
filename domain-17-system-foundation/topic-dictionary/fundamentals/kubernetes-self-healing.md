---
title: Kubernetes Self-Healing（Kubernetes 自愈能力）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- statefulset
- daemonset
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Self-Healing（Kubernetes 自愈能力） 是什么
- 如何 Kubernetes Self-Healing（Kubernetes 自愈能力）
trigger_keywords:
- Kubernetes
- Self-Healing
- 自愈能力
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---



# [[Kubernetes|Kubernetes]] Self-Healing（Kubernetes 自愈能力）

## 概述

Kubernetes 从设计之初就具备自愈能力，以帮助维护工作负载的健康和可用性。当容器失败、节点不可用时，Kubernetes 能够自动进行恢复操作，并确保系统始终维持期望状态。

## 核心概念/原理

自愈的核心思想是：**自动检测问题并采取行动，使当前状态向期望状态恢复**。Kubernetes 通过多个控制平面和节点级组件协同工作，实现不同层面的自动恢复。

## 关键机制或特性

### 容器级重启
- 若 [[concepts/pod-lifecycle.md|pod]] 中的容器失败，[[kubelet|kubelet]] 会根据 Pod 的 `restartPolicy`（如 `Always`、`OnFailure`）自动重启该容器。

### 副本替换
- **Deployment / [[ReplicaSet|ReplicaSet]]**：若某个 Pod 失败，控制器会创建新的 Pod 以维持指定的副本数。
- **[[StatefulSet|StatefulSet]]**：类似地，会重新创建失败的 Pod，并保持稳定的网络标识和存储绑定。
- **DaemonSet**：若 DaemonSet 的 Pod 失败，控制平面会在同一节点上创建替换 Pod，确保节点级服务持续运行。

### 持久存储恢复
- 若运行带有 PersistentVolume（PV）的 Pod 的节点发生问题，Kubernetes 可以将该卷重新挂载到另一个节点上的新 Pod，从而恢复有状态工作负载。

### 服务负载均衡
- 若 Service 后端的某个 Pod 失败，Kubernetes 会自动将其从 Service 的 Endpoints 中移除，确保流量仅路由到健康的 Pod。

### 关键组件
- **kubelet**：确保容器正在运行，并在容器失败时重启它们。
- **Deployment / ReplicaSet / StatefulSet / DaemonSet 控制器**：维护期望的 Pod 副本数量。
- **PersistentVolume 控制器**：管理有状态工作负载的卷挂载和卸载。

## 使用场景

- 容器因临时错误退出后自动恢复
- 节点问题时，自动在其他节点上重新调度工作负载
- 有状态应用（数据库、消息队列等）在节点问题后恢复存储访问
- 服务发现自动剔除不健康后端，保证业务连续性

## 最佳实践/注意事项

- 自愈能力适用于**可恢复的问题**，对于应用程序本身的逻辑错误或数据损坏，仍需单独排查和修复
- 若持久卷本身不可用（如存储后端问题），可能需要额外的恢复步骤
- 合理配置 `restartPolicy`、`livenessProbe` 和 `readinessProbe`，以提升故障检测和恢复效率
- 节点自动扩缩容（Node Autoscaling）也可作为集群层面的自愈手段，在节点问题时自动补充新节点

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/self-healing/

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/annotations.md|注解]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/bpfman.md|bpfman eBPF 管理器]]
