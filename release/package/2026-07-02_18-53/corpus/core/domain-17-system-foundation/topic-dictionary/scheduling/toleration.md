---
title: 容忍
description: Toleration（容忍）是应用在 Pod 上的属性，允许 Pod 被调度到具有匹配污点（Taint）的节点上。它与污点配合工作，控制
  Pod 的调度行为。...
summary: Toleration（容忍）是应用在 Pod 上的属性，允许 Pod 被调度到具有匹配污点（Taint）的节点上。它与污点配合工作，控制 Pod
  的调度行为。...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- toleration
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容忍 是什么
- Toleration 详解
trigger_keywords:
- 容忍
- Toleration
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容忍

> **英文名**: Toleration

## 概述

Toleration（容忍）是应用在 Pod 上的属性，允许 Pod 被调度到具有匹配污点（Taint）的节点上。它与污点配合工作，控制 Pod 的调度行为。

## 核心概念/原理

### 基本语法

```yaml
tolerations:
- key: "gpu"
  operator: "Equal"
  value: "true"
  effect: "NoSchedule"
- key: "node.kubernetes.io/not-ready"
  operator: "Exists"
  effect: "NoExecute"
  tolerationSeconds: 300
```

### 操作符

- **Equal**（默认）：key 和 value 都匹配。
- **Exists**：只匹配 key，忽略 value。
- 空 key + Exists 操作符：匹配所有污点。

## 关键机制或特性

- DaemonSet Pod 通常自动添加系统污点的容忍度。
- `tolerationSeconds` 仅在 `NoExecute` 效果下有效。
- 多个 Toleration 可以匹配多个 Taint。

## 使用场景与最佳实践

- 关键系统组件（如监控代理）添加 `NoExecute` 容忍以确保始终运行。
- 为 `not-ready` 和 `unreachable` 设置合理的 `tolerationSeconds`。
- 不要为普通应用添加过于宽泛的容忍度。

## 参考链接

- [Toleration - Official Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/resource-request.md|Resource Request]]


<!-- risk-assessed -->
