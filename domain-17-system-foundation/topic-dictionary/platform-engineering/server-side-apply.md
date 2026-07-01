---
title: 服务器端应用
description: 'Server-Side Apply（SSA，服务器端应用）是 Kubernetes 中管理资源对象的声明式方式。与 Client-Side Apply 不同，S...'
category: dictionary
tags:
- k8s
- glossary
- platform
- ssa
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务器端应用 是什么
- SSA (Server-Side Apply) 详解
trigger_keywords:
- 服务器端应用
- SSA (Server-Side Apply)
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 服务器端应用

> **英文名**: SSA (Server-Side Apply)

## 概述

Server-Side Apply（SSA，服务器端应用）是 Kubernetes 中管理资源对象的声明式方式。与 Client-Side Apply 不同，SSA 在 API Server 端执行合并逻辑，支持多管理者（managers）协作管理同一资源。

## 核心概念/原理

### 核心概念

- **Field Manager**：标识管理资源的客户端（如 `kubectl`, `helm`, `argocd`）。
- **Managed Fields**：记录每个字段由哪个 Manager 管理。
- **冲突检测**：当多个 Manager 修改同一字段时检测冲突。

### 使用方式

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 服务器端 Apply
kubectl apply --server-side -f deployment.yaml

# 强制覆盖冲突字段
kubectl apply --server-side --force-conflicts -f deployment.yaml

# 在 Manifest 中标识管理者
# 通过 managedFields 自动记录
```

## 关键机制或特性

- SSA 从 K8s v1.22 起达到 stable。
- Client-Side Apply（`kubectl apply`）使用客户端的 last-applied 注解。
- SSA 使用 `managedFields` 替代 `last-applied-configuration` 注解。
- SSA 更适合多工具协作的场景（GitOps、Operator 等）。

## 使用场景与最佳实践

- 新项目和 CI/CD 流水线优先使用 `--server-side`。
- 使用 SSA 的冲突检测避免配置漂移。
- 在 Helm/Kustomize 中启用 SSA 模式。
- 迁移到 SSA 时注意清理旧的 `last-applied-configuration` 注解。

## 参考链接

- [SSA (Server-Side Apply) - Official Documentation](https://kubernetes.io/docs/reference/using-api/server-side-apply/)

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-group.md|Api Group]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-version.md|Api Version]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kind.md|Kind]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/manifest.md|Manifest]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resource.md|Custom Resource]]
