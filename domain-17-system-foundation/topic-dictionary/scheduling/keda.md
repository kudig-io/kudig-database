---
title: KEDA
description: KEDA（Kubernetes Event-Driven Autoscaling）是 CNCF 毕业项目，为 Kubernetes 工作负载提供基于事件驱动的自...
summary: KEDA（Kubernetes Event-Driven Autoscaling）是 CNCF 毕业项目，为 Kubernetes 工作负载提供基于事件驱动的自...
category: dictionary
tags:
- k8s
- glossary
- keda
- autoscaling
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KEDA 是什么
- KEDA (Kubernetes Event-Driven Autoscaling) 详解
trigger_keywords:
- KEDA
- KEDA (Kubernetes Event-Driven Autoscaling)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KEDA

> **英文名**: KEDA (Kubernetes Event-Driven Autoscaling)

## 概述

KEDA（Kubernetes Event-Driven Autoscaling）是 CNCF 毕业项目，为 Kubernetes 工作负载提供基于事件驱动的自动扩缩容能力。它扩展了 HPA，支持 Kafka、RabbitMQ、Prometheus 等 50+ 种外部事件源。

## 核心概念/原理

### 核心架构

- **ScaledObject**：定义扩缩目标和触发器。
- **ScaledJob**：为 Job/CronJob 定义事件驱动的批量处理。
- **Scaler**：事件源适配器（Kafka/Prometheus/SQL 等）。
- **Metrics Adapter**：向 K8s HPA 暴露外部指标。

### 与 HPA 对比

| 特性 | HPA | KEDA |
|------|-----|------|
| 指标源 | CPU/Memory/Custom | 50+ 外部事件源 |
| 缩到零 | 不支持（除 Custom） | 支持 |
| 事件驱动 | 间接 | 原生 |

## 关键机制或特性

- **Scale-to-Zero**：无事件时将 Pod 缩到 0，节省资源。
- **丰富的 Scaler**：Kafka lag、Prometheus 指标、数据库队列长度等。
- **ScaledJob**：按消息队列积压量批量创建 Job 消费者。
- **Fallback**：Scaler 故障时的降级策略。
- 兼容标准 HPA 的 min/max/desired 语义。

## 使用场景与最佳实践

- 消费者类工作负载（消息队列处理）使用 KEDA 替代 HPA。
- 配置 Kafka lag 触发器实现自动消费扩缩容。
- 使用 Scale-to-Zero 降低非高峰时段的资源成本。
- 配合 ScaledJob 处理批量异步任务。
- 设置合理的 cooldownPeriod 避免频繁扩缩。

## 参考链接

- [KEDA Official](https://keda.sh/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/hpa.md|HPA]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/vpa.md|VPA]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/workloads/job.md|Job]]


<!-- risk-assessed -->
