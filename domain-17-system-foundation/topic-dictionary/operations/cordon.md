---
title: 封锁节点
description: 'Cordon 是将节点标记为不可调度的操作。被封锁的节点不会接受新的 Pod 调度，但已运行的 Pod 不受影响。...'
category: dictionary
tags:
- k8s
- glossary
- operations
- node
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 封锁节点 是什么
- Cordon 详解
trigger_keywords:
- 封锁节点
- Cordon
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 封锁节点

> **英文名**: Cordon

## 概述

Cordon 是将节点标记为不可调度的操作。被封锁的节点不会接受新的 Pod 调度，但已运行的 Pod 不受影响。

## 核心概念/原理

### 命令

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

```bash
# 封锁节点
kubectl cordon <node-name>

# 查看节点状态（SchedulingDisabled 表示已封锁）
kubectl get nodes
```

### 节点状态

被封锁的节点会显示 `SchedulingDisabled` 状态，调度器不会再将 Pod 分配到该节点。

## 关键机制或特性

- Cordon 只影响调度，不影响已运行的 Pod。
- 节点上会添加 `node.kubernetes.io/unschedulable` 污点。
- Uncordon 可以恢复节点的可调度状态。

## 使用场景与最佳实践

- 维护节点前先执行 Cordon 阻止新 Pod 调度。
- 配合 Drain 完成节点上 Pod 的安全迁移。
- 使用 `kubectl get nodes` 确认节点状态后再进行维护。

## 参考链接

- [Cordon - Official Documentation](https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/operations/uncordon.md|Uncordon]]
- [[domain-17-system-foundation/topic-dictionary/operations/drain.md|Drain]]
