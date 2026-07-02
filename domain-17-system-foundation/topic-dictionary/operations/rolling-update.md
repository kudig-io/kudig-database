---
title: 滚动更新
description: Rolling Update（滚动更新）是 Kubernetes 中逐步替换旧版本 Pod 为新版本 Pod 的部署策略。它确保在更新过程中始终保持应用可用，实...
summary: Rolling Update（滚动更新）是 Kubernetes 中逐步替换旧版本 Pod 为新版本 Pod 的部署策略。它确保在更新过程中始终保持应用可用，实...
category: dictionary
tags:
- k8s
- glossary
- operations
- deployment
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 滚动更新 是什么
- Rolling Update 详解
trigger_keywords:
- 滚动更新
- Rolling Update
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 滚动更新

> **英文名**: Rolling Update

## 概述

Rolling Update（滚动更新）是 Kubernetes 中逐步替换旧版本 Pod 为新版本 Pod 的部署策略。它确保在更新过程中始终保持应用可用，实现零停机发布。

## 核心概念/原理

### 工作原理

```
1. 创建新 ReplicaSet，启动新 Pod（数量受 maxSurge 限制）
2. 新 Pod 就绪后，减少旧 ReplicaSet 的 Pod（数量受 maxUnavailable 限制）
3. 重复步骤 1-2，直到所有 Pod 更新完成
```

### 配置参数

```yaml
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxSurge: 25%        # 最多超出期望副本数的 Pod 数量
    maxUnavailable: 25%  # 最多不可用的 Pod 数量

```

## 关键机制或特性

- `maxSurge` 和 `maxUnavailable` 控制更新速度。
- `minReadySeconds` 确保新 Pod 稳定运行后才继续更新。
- 可以通过 `kubectl rollout pause/resume` 暂停和恢复更新。
- `kubectl rollout undo` 回滚到上一个版本。

## 使用场景与最佳实践

- 生产环境使用 Rolling Update 确保零停机。
- 设置 `maxUnavailable: 0` 实现严格可用（更新更慢但更安全）。
- 使用 `minReadySeconds` 验证新 Pod 稳定性。
- 配合 Readiness Probe 确保新 Pod 就绪后才继续。

## 参考链接

- [Rolling Update - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/#rolling-update-deployment)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/operations/cordon.md|Cordon]]
- [[domain-17-system-foundation/topic-dictionary/operations/uncordon.md|Uncordon]]

```

<!-- risk-assessed -->
