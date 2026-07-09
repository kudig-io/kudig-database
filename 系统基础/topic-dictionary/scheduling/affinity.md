---
title: 亲和性
description: Affinity（亲和性）是 Kubernetes 中表达 Pod 对节点或其他 Pod 调度偏好的机制。它比 nodeSelector
  更灵活，支持多种操作符...
summary: Affinity（亲和性）是 Kubernetes 中表达 Pod 对节点或其他 Pod 调度偏好的机制。它比 nodeSelector 更灵活，支持多种操作符...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- affinity
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 亲和性 是什么
- Affinity 详解
trigger_keywords:
- 亲和性
- Affinity
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 亲和性

> **英文名**: Affinity

## 概述

Affinity（亲和性）是 Kubernetes 中表达 Pod 对节点或其他 Pod 调度偏好的机制。它比 nodeSelector 更灵活，支持多种操作符和软硬约束。

## 核心概念/原理

### 亲和性类型

#### Node Affinity（节点亲和性）

- **requiredDuringSchedulingIgnoredDuringExecution**：硬性要求（等同于增强的 nodeSelector）。
- **preferredDuringSchedulingIgnoredDuringExecution**：软性偏好（调度器尽量满足，但不保证）。

```yaml
affinity:
  nodeAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
      nodeSelectorTerms:
      - matchExpressions:
        - key: topology.kubernetes.io/zone
          operator: In
          values: [us-east-1a, us-east-1b]
```

#### Pod Affinity（Pod 亲和性）

将 Pod 调度到与特定 Pod 相同拓扑域的位置。

## 关键机制或特性

- 支持的操作符：`In`、`NotIn`、`Exists`、`DoesNotExist`、`Gt`、`Lt`。
- Pod Affinity/Anti-Affinity 可以基于 `topologyKey` 指定拓扑域。
- 软性偏好通过 `weight` 字段控制优先级。

## 使用场景与最佳实践

- 使用 Node Affinity 将工作负载调度到特定硬件或区域的节点。
- 使用 Pod Affinity 将相关服务调度到一起减少延迟。
- 软性偏好（preferred）在无法满足时不会阻止 Pod 调度。

## 参考链接

- [Affinity - Official Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)

## Related

- [[系统基础/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[系统基础/topic-dictionary/scheduling/taint.md|Taint]]
- [[系统基础/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[系统基础/topic-dictionary/scheduling/node-selector.md|Node Selector]]
- [[系统基础/topic-dictionary/scheduling/resource-request.md|Resource Request]]


<!-- risk-assessed -->
