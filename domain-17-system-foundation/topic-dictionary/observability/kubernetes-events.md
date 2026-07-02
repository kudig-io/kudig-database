---
title: Kubernetes 事件
description: Kubernetes Events（事件）是集群中发生的操作和状态变更的记录。事件由各个控制器和 kubelet 生成，提供了集群运行的实时视图，是排查问题的重...
summary: Kubernetes Events（事件）是集群中发生的操作和状态变更的记录。事件由各个控制器和 kubelet 生成，提供了集群运行的实时视图，是排查问题的重...
category: dictionary
tags:
- k8s
- glossary
- observability
- events
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 事件 是什么
- Kubernetes Events 详解
trigger_keywords:
- Kubernetes 事件
- Kubernetes Events
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 事件

> **英文名**: Kubernetes Events

## 概述

Kubernetes Events（事件）是集群中发生的操作和状态变更的记录。事件由各个控制器和 kubelet 生成，提供了集群运行的实时视图，是排查问题的重要信息来源。

## 核心概念/原理

### 事件结构

每个 Event 包含：
- **Type**：`Normal` 或 `Warning`。
- **Reason**：事件原因（如 `Scheduled`, `Pulled`, `FailedScheduling`）。
- **Message**：详细描述。
- **Source**：事件来源组件。
- **Count**：事件发生次数。
- **FirstTimestamp/LastTimestamp**：首次和最后发生时间。

### 查看事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看命名空间事件
kubectl get events -n default --sort-by='.lastTimestamp'

# 查看特定 Pod 的事件
kubectl describe pod <pod-name> -n default

# 使用 fieldSelector 过滤
kubectl get events --field-selector type=Warning

# 使用 --watch 实时监控
kubectl get events -w
```
## 关键机制或特性

- Event 默认 TTL 为 1 小时（可通过 `--event-ttl` 调整）。
- Event 存储在 etcd 中，大量 Event 可能影响 etcd 性能。
- Event API（`events.k8s.io/v1`）替代了旧的 core/v1 Event API。
- Event 不会持久化（TTL 过期后自动删除）。

## 使用场景与最佳实践

- 排查问题时首先查看相关资源的事件。
- 使用 `--field-selector type=Warning` 快速定位异常事件。
- 配置 Event 导出工具（如 k8s-event-exporter）将事件发送到外部系统。
- 使用 `kubectl get events --watch` 实时监控集群变更。

## 参考链接

- [Kubernetes Events - Official Documentation](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]
- [[domain-17-system-foundation/topic-dictionary/observability/metrics-server.md|Metrics Server]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging.md|Logging]]


<!-- risk-assessed -->
