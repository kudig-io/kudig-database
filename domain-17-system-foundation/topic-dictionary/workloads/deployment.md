---
title: Deployment
description: Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod
  副本数和版本，支持...
summary: Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod 副本数和版本，支持...
category: dictionary
tags:
- k8s
- glossary
- deployment
- workload
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Deployment 是什么
- Deployment 详解
trigger_keywords:
- Deployment
- dictionary
prerequisites:
- kubectl-basics
---



# Deployment

> **英文名**: Deployment

## 概述

Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod 副本数和版本，支持声明式更新、滚动发布和回滚。

## 核心概念/原理

### 核心能力

- **声明式更新**：修改 Pod 模板后，Deployment 自动执行滚动更新。
- **版本管理**：每次更新创建新的 ReplicaSet，保留历史记录支持回滚。
- **滚动更新策略**：通过 `maxSurge` 和 `maxUnavailable` 控制更新节奏。
- **扩缩容**：修改 `replicas` 字段即可调整副本数。

### 更新流程

```
修改 Pod 模板 → 创建新 ReplicaSet → 逐步增加新 Pod → 逐步减少旧 Pod → 完成更新
```

## 关键机制或特性

- `strategy.type: RollingUpdate` 是最常用的更新策略，保证零停机。
- `strategy.type: Recreate` 先停掉所有旧 Pod 再创建新 Pod，适用于不兼容版本升级。
- `revisionHistoryLimit` 控制保留的历史 ReplicaSet 数量（默认 10）。
- `minReadySeconds` 确保新 Pod 就绪后才继续更新。

## 使用场景与最佳实践

- 生产环境始终使用 Deployment 而非裸 ReplicaSet 管理应用。
- 设置合理的 `maxSurge` 和 `maxUnavailable`（推荐 25%/25%）。
- 使用 `kubectl rollout status` 监控更新进度。
- 配置 Pod 的反亲和性，确保副本分布在不同的节点/可用区。

## 参考链接

- [Deployment - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)

## Related

[[entities/deployment.md|Deployment]]
