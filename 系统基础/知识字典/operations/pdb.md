---
title: Pod 中断预算
description: Pod Disruption Budget（PDB，Pod 中断预算）是 Kubernetes 中用于限制同时被自愿中断的 Pod 数量的策略资源。它确保在节点...
summary: Pod Disruption Budget（PDB，Pod 中断预算）是 Kubernetes 中用于限制同时被自愿中断的 Pod 数量的策略资源。它确保在节点...
category: dictionary
tags:
- k8s
- glossary
- operations
- pdb
- reliability
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 中断预算 是什么
- PDB (Pod Disruption Budget) 详解
trigger_keywords:
- Pod 中断预算
- PDB (Pod Disruption Budget)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod 中断预算

> **英文名**: PDB (Pod Disruption Budget)

## 概述

Pod Disruption Budget（PDB，Pod 中断预算）是 Kubernetes 中用于限制同时被自愿中断的 Pod 数量的策略资源。它确保在节点维护、集群升级等操作期间，应用始终保持最低可用水平。

## 核心概念/原理

### 核心概念

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2         # 至少保持 2 个 Pod 可用
  # 或
  maxUnavailable: 1       # 最多允许 1 个 Pod 不可用
  selector:
    matchLabels:
      app: web

```

### 自愿中断 vs 非自愿中断

- **自愿中断**：Drain、节点维护、集群升级（受 PDB 保护）。
- **非自愿中断**：节点故障、OOM Kill、驱逐（不受 PDB 保护）。

### PDB 的作用时机

PDB 在以下场景中阻止 Pod 被驱逐：
- `kubectl drain` 操作。
- 集群升级过程中的 Pod 迁移。
- Cluster Autoscaler 缩容节点。

## 关键机制或特性

- PDB 从 K8s v1.21 起达到 stable。
- `minAvailable` 和 `maxUnavailable` 不能同时设置。
- 支持百分比和绝对数字（如 `minAvailable: 50%`）。
- `unhealthyPodEvictionPolicy`（v1.27+）允许驱逐不健康的 Pod 即使 PDB 不满足。

## 使用场景与最佳实践

- 为所有生产 Deployment/StatefulSet 配置 PDB。
- `maxUnavailable: 1` 适合大多数场景（允许 1 个 Pod 中断）。
- 确保 `minAvailable` 不超过实际副本数减 1。
- 结合滚动更新策略实现零停机部署和维护。
- 监控 PDB 的 `currentHealthy` 和 `disruptionsAllowed` 状态。

## 参考链接

- [PDB (Pod Disruption Budget) - Official Documentation](https://kubernetes.io/docs/tasks/run-application/configure-pdb/)

## Related

- [[系统基础/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[系统基础/topic-dictionary/tooling/helm.md|Helm]]
- [[系统基础/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[系统基础/topic-dictionary/operations/cordon.md|Cordon]]
- [[系统基础/topic-dictionary/operations/uncordon.md|Uncordon]]

```

<!-- risk-assessed -->
