---
title: 优先级类
description: PriorityClass 是 Kubernetes 的 Pod 优先级定义资源，通过 priorityClassName 关联到 Pod，实现高优先级
  Pod...
summary: PriorityClass 是 Kubernetes 的 Pod 优先级定义资源，通过 priorityClassName 关联到 Pod，实现高优先级
  Pod...
category: dictionary
tags:
- k8s
- glossary
- configuration
- scheduling
- preemption
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 优先级类 是什么
- PriorityClass 详解
trigger_keywords:
- 优先级类
- PriorityClass
- dictionary
prerequisites:
- kubernetes
---



# 优先级类（PriorityClass）

## 概述

PriorityClass 是 Kubernetes 的 Pod 优先级定义资源，通过 priorityClassName 关联到 Pod，实现高优先级 Pod 对低优先级 Pod 的抢占（Preemption），保障关键工作负载的调度优先权。

## 核心概念/原理

- **优先级声明**：定义命名空间的 Pod 优先级等级
- **抢占机制**：高优先级 Pod 可驱逐低优先级 Pod
- **全局资源**：PriorityClass 是集群级资源
- **内置优先级**：system-cluster-critical/system-node-critical

## 关键机制或特性

- `value`：优先级数值（越大越优先，最大 10^9）
- `globalDefault`：未指定时是否为默认优先级
- `preemptionPolicy`：PreemptLowerPriority/Never
- `description`：优先级说明
- 系统优先级：2000000000+/1000000000+
- Scheduler 在资源不足时触发抢占
- 抢占过程：找牺牲 Pod → 驱逐 → 调度

## 使用场景与最佳实践

- 生产关键应用的优先级保障
- 批处理任务的低优先级标记
- DaemonSet 的系统级优先级
- 资源竞争时的调度策略
- 最佳实践：分层优先级（系统>生产>测试>批处理）、避免全部高优先级

## 参考链接

- https://kubernetes.io/docs/concepts/scheduling-eviction/pod-priority-preemption/
- https://kubernetes.io/docs/concepts/configuration/pod-priority-preemption/

## Related

- [[domain-17-system-foundation/知识字典/operations/pdb.md|PDB]]
- [[domain-17-system-foundation/知识字典/configuration/priority-class.md|Preemption]]
- [[domain-17-system-foundation/知识字典/configuration/resource-quota.md|ResourceQuota]]
