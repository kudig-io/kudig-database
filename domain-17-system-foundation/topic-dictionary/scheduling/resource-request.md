---
title: 资源请求
description: Resource Request 是容器声明需要的最小资源量。调度器根据 Request 值决定 Pod 应该调度到哪个节点，kubelet
  保证容器至少能获得...
summary: Resource Request 是容器声明需要的最小资源量。调度器根据 Request 值决定 Pod 应该调度到哪个节点，kubelet 保证容器至少能获得...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- resource
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 资源请求 是什么
- Resource Request 详解
trigger_keywords:
- 资源请求
- Resource Request
- dictionary
prerequisites:
- kubectl-basics
---



# 资源请求

> **英文名**: Resource Request

## 概述

Resource Request 是容器声明需要的最小资源量。调度器根据 Request 值决定 Pod 应该调度到哪个节点，kubelet 保证容器至少能获得 Request 数量的资源。

## 核心概念/原理

### 核心概念

```yaml
resources:
  requests:
    cpu: "250m"      # 0.25 核 CPU
    memory: "256Mi"  # 256 MiB 内存
```

- **CPU Request**：调度器确保节点有足够的 CPU 容量。1 CPU = 1000m（millicores）。
- **Memory Request**：调度器确保节点有足够的内存。kubelet 通过 cgroup 保证内存分配。
- **Ephemeral Storage Request**：本地临时存储的请求量。

### 调度影响

调度器使用 Request 值进行调度决策：只有当节点的（总容量 - 已分配 Request）≥ Pod Request 时，Pod 才能被调度到该节点。

## 关键机制或特性

- Request 是调度的依据，Limit 是运行时的上限。
- 未设置 Request 时默认值为 0（BestEffort QoS）。
- 设置过高的 Request 会浪费资源，过低可能导致 OOM 或 CPU 节流。

## 使用场景与最佳实践

- 基于实际监控数据设置合理的 Request 值。
- CPU Request 应覆盖应用的平均负载。
- Memory Request 应覆盖应用的稳态内存使用。
- 使用 VPA（Vertical Pod Autoscaler）推荐值作为参考。

## 参考链接

- [Resource Request - Official Documentation](https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]
