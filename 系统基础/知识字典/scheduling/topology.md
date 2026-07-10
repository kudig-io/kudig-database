---
title: 拓扑
description: Topology（拓扑）在 Kubernetes 中表示节点在物理或逻辑上的位置关系，如区域（Region）、可用区（Zone）、机架（Rack）等。拓扑信息用...
summary: Topology（拓扑）在 Kubernetes 中表示节点在物理或逻辑上的位置关系，如区域（Region）、可用区（Zone）、机架（Rack）等。拓扑信息用...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- topology
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 拓扑 是什么
- Topology 详解
trigger_keywords:
- 拓扑
- Topology
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 拓扑

> **英文名**: Topology

## 概述

Topology（拓扑）在 Kubernetes 中表示节点在物理或逻辑上的位置关系，如区域（Region）、可用区（Zone）、机架（Rack）等。拓扑信息用于调度决策，以实现高可用和故障隔离。

## 核心概念/原理

### 拓扑域

Kubernetes 通过标签表示拓扑信息：

| 标签 | 含义 | 示例 |
|------|------|------|
| `topology.kubernetes.io/region` | 区域 | us-east-1 |
| `topology.kubernetes.io/zone` | 可用区 | us-east-1a |
| `kubernetes.io/hostname` | 节点 | node-1 |
| `topology.kubernetes.io/node` | 节点（推荐） | node-1 |

### 拓扑感知调度

调度器利用拓扑信息实现：
- **Pod 拓扑分布约束**（topologySpreadConstraints）：控制 Pod 在拓扑域间的均匀分布。
- **拓扑感知路由**（Topology Aware Routing）：将流量路由到同拓扑域的后端。
- **存储拓扑感知**：将 PVC 绑定到与 Pod 同区域的存储卷。

## 关键机制或特性

- 云厂商自动为节点添加区域和可用区标签。
- 自定义拓扑域可以通过节点标签实现（如机架标签）。
- `topology.kubernetes.io/*` 标签取代了旧的 `failure-domain.beta.kubernetes.io/*`。

## 使用场景与最佳实践

- 为高可用应用配置跨可用区分布。
- 使用拓扑感知路由减少跨区域流量成本。
- 在有状态应用中考虑存储与 Pod 的拓扑对齐。

## 参考链接

- [Topology - Official Documentation](https://kubernetes.io/docs/reference/labels-annotations-taints/)

## Related

- [[系统基础/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[系统基础/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[系统基础/topic-dictionary/scheduling/taint.md|Taint]]
- [[系统基础/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[系统基础/topic-dictionary/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
