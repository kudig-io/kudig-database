---
title: 拓扑分布约束
description: Pod Topology Spread Constraints（拓扑分布约束）用于控制 Pod 在集群中的分布方式，使其跨故障域（如可用区、节点）均匀分布。这是...
summary: Pod Topology Spread Constraints（拓扑分布约束）用于控制 Pod 在集群中的分布方式，使其跨故障域（如可用区、节点）均匀分布。这是...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- topology
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 拓扑分布约束 是什么
- topologySpreadConstraints 详解
trigger_keywords:
- 拓扑分布约束
- topologySpreadConstraints
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 拓扑分布约束

> **英文名**: topologySpreadConstraints

## 概述

Pod Topology Spread Constraints（拓扑分布约束）用于控制 Pod 在集群中的分布方式，使其跨故障域（如可用区、节点）均匀分布。这是实现高可用部署的关键调度机制。

## 核心概念/原理

### 核心参数

```yaml
topologySpreadConstraints:
- maxSkew: 1                # 最大倾斜度（拓扑域间 Pod 数量最大差值）
  topologyKey: topology.kubernetes.io/zone  # 拓扑域标签
  whenUnsatisfiable: DoNotSchedule  # 不满足时的行为
  labelSelector:
    matchLabels:
      app: web
```

### maxSkew 的含义

maxSkew=1 表示任意两个拓扑域中匹配的 Pod 数量差不超过 1。

### whenUnsatisfiable

- `DoNotSchedule`（默认）：硬性约束，不满足则不调度。
- `ScheduleAnyway`：软性约束，尽量满足但不阻止调度。

## 关键机制或特性

- 从 K8s v1.24 起达到 stable。
- 支持多个约束组合（如同时按 zone 和 hostname 分布）。
- `minDomains` 参数（v1.25+）指定最小拓扑域数量。
- `matchLabelKeys`（v1.27+）可以基于 Pod 标签动态分组。

## 使用场景与最佳实践

- 生产服务配置跨可用区的均匀分布（maxSkew=1, topologyKey=zone）。
- 结合 Pod Anti-Affinity 实现更精细的分布控制。
- 使用 `ScheduleAnyway` 作为降级策略，避免无法满足时 Pod 无法调度。

## 参考链接

- [topologySpreadConstraints - Official Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/topology-spread-constraints/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
