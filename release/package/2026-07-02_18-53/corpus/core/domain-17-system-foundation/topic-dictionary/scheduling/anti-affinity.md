---
title: 反亲和性
description: Anti-Affinity（反亲和性）表达 Pod 不希望与某些 Pod 调度到同一拓扑域的约束。它是实现高可用和故障隔离的关键调度策略。...
summary: Anti-Affinity（反亲和性）表达 Pod 不希望与某些 Pod 调度到同一拓扑域的约束。它是实现高可用和故障隔离的关键调度策略。...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- anti-affinity
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 反亲和性 是什么
- Anti-Affinity 详解
trigger_keywords:
- 反亲和性
- Anti-Affinity
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 反亲和性

> **英文名**: Anti-Affinity

## 概述

Anti-Affinity（反亲和性）表达 Pod 不希望与某些 Pod 调度到同一拓扑域的约束。它是实现高可用和故障隔离的关键调度策略。

## 核心概念/原理

### 核心概念

- **Pod Anti-Affinity**：确保 Pod 不与特定 Pod 运行在同一拓扑域（如同一节点、同一可用区）。
- **硬性约束**：`requiredDuringSchedulingIgnoredDuringExecution` — 必须满足。
- **软性约束**：`preferredDuringSchedulingIgnoredDuringExecution` — 尽量满足。

### 示例：确保副本分布在不同节点

```yaml
affinity:
  podAntiAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
    - labelSelector:
        matchLabels:
          app: web
      topologyKey: kubernetes.io/hostname
```

## 关键机制或特性

- `topologyKey` 定义拓扑域：`kubernetes.io/hostname`（节点级）、`topology.kubernetes.io/zone`（可用区级）。
- 硬性反亲和性可能导致 Pod 无法调度（如果没有足够的拓扑域）。
- 软性反亲和性通过 `weight` 控制优先级。

## 使用场景与最佳实践

- 使用反亲和性确保应用副本分布在不同的节点或可用区。
- 优先使用软性反亲和性，避免过于严格的约束导致调度失败。
- 结合 topologySpreadConstraints 获得更均匀的分部效果。

## 参考链接

- [Anti-Affinity - Official Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)

## Related

- [[domain-17-system-foundation/知识字典/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/知识字典/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/知识字典/scheduling/node-selector.md|Node Selector]]
- [[domain-17-system-foundation/知识字典/scheduling/resource-request.md|Resource Request]]
- [[domain-17-system-foundation/知识字典/scheduling/resource-limit.md|Resource Limit]]


<!-- risk-assessed -->
