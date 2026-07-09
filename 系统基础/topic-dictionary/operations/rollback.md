---
title: 回滚
description: Rollback（回滚）是将 Deployment 恢复到之前版本的操作。当新版本出现问题时，可以快速回退到已知的工作版本。...
summary: Rollback（回滚）是将 Deployment 恢复到之前版本的操作。当新版本出现问题时，可以快速回退到已知的工作版本。...
category: dictionary
tags:
- k8s
- glossary
- operations
- deployment
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 回滚 是什么
- Rollback 详解
trigger_keywords:
- 回滚
- Rollback
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 回滚

> **英文名**: Rollback

## 概述

Rollback（回滚）是将 Deployment 恢复到之前版本的操作。当新版本出现问题时，可以快速回退到已知的工作版本。

## 核心概念/原理

### 命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看更新历史
kubectl rollout history deployment/<name>

# 回滚到上一个版本
kubectl rollout undo deployment/<name>

# 回滚到指定版本
kubectl rollout undo deployment/<name> --to-revision=3

# 查看回滚状态
kubectl rollout status deployment/<name>
```
## 关键机制或特性

- `revisionHistoryLimit` 控制保留的历史 ReplicaSet 数量（默认 10）。
- 超出 `revisionHistoryLimit` 的旧 ReplicaSet 会被删除，无法回滚。
- 回滚操作本身也会创建一个新的 revision。

## 使用场景与最佳实践

- 设置合理的 `revisionHistoryLimit`（生产建议 5-10）。
- 更新前记录变更内容，便于排查需要回滚的原因。
- 回滚后验证应用功能和指标是否正常。

## 参考链接

- [Rollback - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/#rolling-back-a-deployment)

## Related

- [[系统基础/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[系统基础/topic-dictionary/tooling/helm.md|Helm]]
- [[系统基础/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[系统基础/topic-dictionary/operations/cordon.md|Cordon]]
- [[系统基础/topic-dictionary/operations/uncordon.md|Uncordon]]


<!-- risk-assessed -->
