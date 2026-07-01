---
title: 资源限制
description: Resource Limit 是容器允许使用的最大资源量。当容器使用量超过 Limit 时，CPU 会被节流（throttle），内存超出则容器会被
  OOM K...
summary: Resource Limit 是容器允许使用的最大资源量。当容器使用量超过 Limit 时，CPU 会被节流（throttle），内存超出则容器会被
  OOM K...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- resource
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 资源限制 是什么
- Resource Limit 详解
trigger_keywords:
- 资源限制
- Resource Limit
- dictionary
prerequisites:
- kubectl-basics
---



# 资源限制

> **英文名**: Resource Limit

## 概述

Resource Limit 是容器允许使用的最大资源量。当容器使用量超过 Limit 时，CPU 会被节流（throttle），内存超出则容器会被 OOM Kill。

## 核心概念/原理

### 核心概念

```yaml
resources:
  requests:
    cpu: "250m"
    memory: "256Mi"
  limits:
    cpu: "500m"
    memory: "512Mi"
```

- **CPU Limit**：超出 Limit 时 CPU 被节流（不是 Kill），性能下降但不会重启。
- **Memory Limit**：超出 Limit 时容器被 OOM Kill（Out of Memory），根据 restartPolicy 可能重启。
- **规则**：Limit 必须 ≥ Request。

## 关键机制或特性

- CPU Limit 通过 CFS（Completely Fair Scheduler）配额实现。
- Memory Limit 通过 cgroup 的 memory.limit_in_bytes 实现。
- `LimitRange` 可以为命名空间设置默认的 Request/Limit。

## 使用场景与最佳实践

- 始终设置 Memory Limit 防止容器耗尽节点内存。
- CPU Limit 的设置需要谨慎：过低的 CPU Limit 会导致请求延迟增加。
- 使用 `requests == limits` 实现 Guaranteed QoS（关键工作负载）。
- 监控 OOMKill 事件和 CPU 节流指标。

## 参考链接

- [Resource Limit - Official Documentation](https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]
