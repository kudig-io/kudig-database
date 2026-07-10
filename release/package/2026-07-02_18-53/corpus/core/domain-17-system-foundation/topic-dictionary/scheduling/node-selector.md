---
title: 节点选择器
description: nodeSelector 是 Kubernetes 中最简单的 Pod 调度约束方式。它通过键值对标签匹配，将 Pod 限制在具有特定标签的节点上运行。...
summary: nodeSelector 是 Kubernetes 中最简单的 Pod 调度约束方式。它通过键值对标签匹配，将 Pod 限制在具有特定标签的节点上运行。...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- node-selector
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点选择器 是什么
- nodeSelector 详解
trigger_keywords:
- 节点选择器
- nodeSelector
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点选择器

> **英文名**: nodeSelector

## 概述

nodeSelector 是 Kubernetes 中最简单的 Pod 调度约束方式。它通过键值对标签匹配，将 Pod 限制在具有特定标签的节点上运行。

## 核心概念/原理

### 基本用法

```yaml
spec:
  nodeSelector:
    disktype: ssd
    zone: us-east-1a
```

Pod 只会被调度到同时具有 `disktype=ssd` 和 `zone=us-east-1a` 标签的节点。

### 局限性

- 只支持等值匹配（不支持 In、NotIn、Exists 等操作符）。
- 无法表达"偏好"（soft requirement），只有"必须"（hard requirement）。
- 复杂场景应使用 nodeAffinity。

## 关键机制或特性

- nodeSelector 是 nodeAffinity 的简化版本。
- 空 `nodeSelector: {}` 表示不限制。
- 可以与其他调度约束（Affinity、Taint/Toleration）组合使用。

## 使用场景与最佳实践

- 简单的节点约束场景使用 nodeSelector（语法简洁）。
- 复杂场景使用 nodeAffinity。
- 为节点设置规范的标签体系，便于调度管理。

## 参考链接

- [nodeSelector - Official Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)

## Related

- [[domain-17-system-foundation/知识字典/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/知识字典/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/知识字典/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/知识字典/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/知识字典/scheduling/resource-request.md|Resource Request]]


<!-- risk-assessed -->
