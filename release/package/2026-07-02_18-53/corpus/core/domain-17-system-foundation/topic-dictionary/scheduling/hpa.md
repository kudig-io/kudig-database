---
title: 水平 Pod 自动扩缩容
description: HPA（Horizontal Pod Autoscaler，水平 Pod 自动扩缩容）是 Kubernetes 中根据观测到的指标自动调整
  Pod 副本数量的控...
summary: HPA（Horizontal Pod Autoscaler，水平 Pod 自动扩缩容）是 Kubernetes 中根据观测到的指标自动调整 Pod
  副本数量的控...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- autoscaling
- hpa
tier: core
created: 2026-05
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 水平 Pod 自动扩缩容 是什么
- HPA (Horizontal Pod Autoscaler) 详解
trigger_keywords:
- 水平 Pod 自动扩缩容
- HPA (Horizontal Pod Autoscaler)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 水平 Pod 自动扩缩容

> **英文名**: HPA (Horizontal Pod Autoscaler)

## 概述

HPA（Horizontal Pod Autoscaler，水平 Pod 自动扩缩容）是 Kubernetes 中根据观测到的指标自动调整 Pod 副本数量的控制器。它通过增加或减少副本数来应对负载变化。

## 核心概念/原理

### 核心机制

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

### 支持的指标类型

- **Resource**：CPU/内存利用率。
- **Pods**：自定义 Pod 级指标（如 QPS）。
- **Object**：与特定对象关联的指标。
- **External**：集群外部指标。
- **Container Resource**：容器级资源指标。

## 关键机制或特性

- HPA 依赖 Metrics Server（Resource 指标）或 Prometheus Adapter（自定义指标）。
- 默认同步周期 15 秒（`--horizontal-pod-autoscaler-sync-period`）。
- 扩缩容有冷却时间：缩容默认 5 分钟，扩容默认 3 分钟。
- `behavior` 字段（v1.23+）提供精细的扩缩容行为控制。

## 使用场景与最佳实践

- 为生产服务配置 HPA 实现自动扩缩容。
- 设置合理的 minReplicas（至少 2 保证高可用）和 maxReplicas。
- 结合自定义指标（如请求延迟、队列长度）实现更精准的扩缩。
- 配置 `behavior.scaleDown.stabilizationWindowSeconds` 防止频繁缩容抖动。
- 监控 HPA 的当前状态和扩缩容历史。

## 参考链接

- [HPA (Horizontal Pod Autoscaler) - Official Documentation](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
