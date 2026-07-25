---
title: 清单
description: Manifest 是 Kubernetes 资源的声明式定义文件，通常使用 YAML 或 JSON 格式。它描述了资源的期望状态，Kubernetes
  会持续将...
summary: Manifest 是 Kubernetes 资源的声明式定义文件，通常使用 YAML 或 JSON 格式。它描述了资源的期望状态，Kubernetes
  会持续将...
category: dictionary
tags:
- k8s
- glossary
- manifest
- yaml
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 清单 是什么
- Manifest 详解
trigger_keywords:
- 清单
- Manifest
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 清单

> **英文名**: Manifest

## 概述

Manifest 是 Kubernetes 资源的声明式定义文件，通常使用 YAML 或 JSON 格式。它描述了资源的期望状态，Kubernetes 会持续将当前状态向期望状态调整。

## 核心概念/原理

### 基本结构

```yaml
apiVersion: apps/v1       # API 版本
kind: Deployment           # 资源类型
metadata:                  # 元数据（名称、命名空间、标签）
  name: my-app
  namespace: default
spec:                      # 期望状态（规格）
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: app
        image: nginx:1.25

```

### 管理方式

- **命令式**：`kubectl run`、`kubectl create`。
- **声明式**：`kubectl apply -f manifest.yaml`（推荐）。
- **Kustomize**：通过 overlay 管理 Manifest 变体。
- **Helm**：通过模板和 Values 动态生成 Manifest。

## 关键机制或特性

- `kubectl apply` 使用 Server-Side Apply（SSA）或 Client-Side Apply 管理资源。
- Manifest 应纳入版本控制（GitOps 理念）。
- `kubectl diff` 可以预览 Manifest 变更。

## 使用场景与最佳实践

- 始终使用声明式管理（`kubectl apply`）而非命令式。
- 将 Manifest 存储在 Git 仓库中。
- 使用 Kustomize 或 Helm 管理多环境差异。
- 为 Manifest 添加完整的 labels 和 annotations。

## 参考链接

- [Manifest - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/)

## Related

- [[17-系统基础/06-知识字典/platform-engineering/api-group.md|Api Group]]
- [[17-系统基础/06-知识字典/platform-engineering/api-version.md|Api Version]]
- [[17-系统基础/06-知识字典/platform-engineering/kind.md|Kind]]
- [[17-系统基础/06-知识字典/platform-engineering/custom-resource.md|Custom Resource]]
- [[17-系统基础/06-知识字典/platform-engineering/operator-pattern.md|Operator Pattern]]

```

<!-- risk-assessed -->
