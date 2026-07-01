---
title: 驱逐
description: Drain 是安全地将节点上的 Pod 迁移到其他节点的操作。它会先 Cordon 节点，然后逐个驱逐节点上的 Pod（尊重 PDB），确保应用的可用性。...
summary: Drain 是安全地将节点上的 Pod 迁移到其他节点的操作。它会先 Cordon 节点，然后逐个驱逐节点上的 Pod（尊重 PDB），确保应用的可用性。...
category: dictionary
tags:
- k8s
- glossary
- operations
- node
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 驱逐 是什么
- Drain 详解
trigger_keywords:
- 驱逐
- Drain
- dictionary
prerequisites:
- kubectl-basics
---



# 驱逐

> **英文名**: Drain

## 概述

Drain 是安全地将节点上的 Pod 迁移到其他节点的操作。它会先 Cordon 节点，然后逐个驱逐节点上的 Pod（尊重 PDB），确保应用的可用性。

## 核心概念/原理

### 命令

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 驱逐节点（自动 cordon + 驱逐 Pod）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 常用参数
--ignore-daemonsets    # 忽略 DaemonSet Pod（它们会被重新创建）
--delete-emptydir-data # 删除使用 emptyDir 的 Pod
--force                # 强制删除不受控制器管理的 Pod
--grace-period=30      # 优雅关闭等待时间
--timeout=5m           # 驱逐超时时间
```

## 关键机制或特性

- Drain 会尊重 PodDisruptionBudget（PDB），不会同时驱逐过多 Pod。
- DaemonSet Pod 不会被驱逐（除非使用 `--force`）。
- 没有控制器管理的裸 Pod 不会被驱逐（除非使用 `--force`）。

## 使用场景与最佳实践

- 节点维护前始终执行 Drain。
- 使用 `--ignore-daemonsets` 避免 DaemonSet Pod 阻塞 Drain。
- 监控 Drain 进度，确保 PDB 允许驱逐。
- 大规模集群中批量 Drain 时要注意 PDB 和资源容量。

## 参考链接

- [Drain - Official Documentation](https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/operations/cordon.md|Cordon]]
- [[domain-17-system-foundation/topic-dictionary/operations/uncordon.md|Uncordon]]
