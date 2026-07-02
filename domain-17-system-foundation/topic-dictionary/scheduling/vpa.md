---
title: 垂直 Pod 自动扩缩容
description: VPA（Vertical Pod Autoscaler，垂直 Pod 自动扩缩容）是 Kubernetes 中自动调整 Pod 的资源 Request/Limi...
summary: VPA（Vertical Pod Autoscaler，垂直 Pod 自动扩缩容）是 Kubernetes 中自动调整 Pod 的资源 Request/Limi...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- autoscaling
- vpa
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 垂直 Pod 自动扩缩容 是什么
- VPA (Vertical Pod Autoscaler) 详解
trigger_keywords:
- 垂直 Pod 自动扩缩容
- VPA (Vertical Pod Autoscaler)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 垂直 Pod 自动扩缩容

> **英文名**: VPA (Vertical Pod Autoscaler)

## 概述

VPA（Vertical Pod Autoscaler，垂直 Pod 自动扩缩容）是 Kubernetes 中自动调整 Pod 的资源 Request/Limit 的工具。它根据容器的历史资源使用情况推荐或自动调整资源配置。

## 核心概念/原理

### 工作模式

| 模式 | 行为 |
|------|------|
| `Off` | 只推荐，不自动调整 |
| `Initial` | 仅在 Pod 创建时设置资源 |
| `Recreate` | 自动调整，需要重启 Pod |
| `Auto` | 自动调整（目前等同于 Recreate） |

### 示例

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Auto"
```

## 关键机制或特性

- VPA 是 `autoscaling.k8s.io` API Group 下的资源。
- VPA 和 HPA 不能同时作用于同一个 Pod 的同一指标（CPU/Memory）。
- VPA 使用 Recommender 组件分析历史数据并推荐资源值。
- `Recreate` 模式需要重启 Pod 以应用新资源值。

## 使用场景与最佳实践

- 使用 VPA 的 `Off` 模式获取资源推荐值，手动调整后应用。
- 与 HPA 结合使用：VPA 调整单 Pod 资源，HPA 调整副本数。
- 对于不能重启的关键服务，使用 `Initial` 模式。
- 监控 VPA 推荐值与实际使用值的偏差。

## 参考链接

- [VPA (Vertical Pod Autoscaler) - Official Documentation](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
