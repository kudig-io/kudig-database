---
title: Kustomize
description: Kustomize 是 Kubernetes 原生的配置管理工具，通过 overlay 模式对 YAML 资源进行无模板的定制。它已内置到
  kubectl（`k...
summary: Kustomize 是 Kubernetes 原生的配置管理工具，通过 overlay 模式对 YAML 资源进行无模板的定制。它已内置到 kubectl（`k...
category: dictionary
tags:
- k8s
- glossary
- kustomize
- configuration
- gitops
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kustomize 是什么
- Kustomize 详解
trigger_keywords:
- Kustomize
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kustomize

> **英文名**: Kustomize

## 概述

Kustomize 是 Kubernetes 原生的配置管理工具，通过 overlay 模式对 YAML 资源进行无模板的定制。它已内置到 kubectl（`kubectl apply -k`），是 Kubernetes 官方推荐的配置管理方案之一。

## 核心概念/原理

### 核心概念

- **Base**：基础配置（通用 YAML）。
- **Overlay**：环境特定的修改层（dev/staging/prod）。
- **kustomization.yaml**：定义 bases、patches、generators 的配置文件。

### 与 Helm 对比

| 特性 | Kustomize | Helm |
|------|-----------|------|
| 模板 | 无（YAML 叠加） | Go 模板 |
| 复杂度 | 低 | 中 |
| 包管理 | 无 | Chart/Release |
| 适用场景 | 配置微调 | 应用打包分发 |

## 关键机制或特性

- **Patches**：Strategic Merge Patch 和 JSON Patch 两种模式。
- **Generators**：ConfigMap/Secret 自动生成（带内容哈希）。
- **Components**：可复用的配置片段。
- **内置到 kubectl**：`kubectl apply -k <dir>` 直接使用。
- **Transformers**：全局修改 labels、namespaces、name prefixes。

## 使用场景与最佳实践

- 多环境配置管理使用 Kustomize overlay。
- 配合 Argo CD/Flux 实现 GitOps 配置渲染。
- 为 ConfigMap/Secret 使用 Generator 自动加 hash 触发滚动更新。
- 使用 Components 提取跨环境共享的配置片段。
- 复杂应用打包考虑 Helm，精细配置调整使用 Kustomize。

## 参考链接

- [Kustomize Official](https://kubectl.docs.kubernetes.io/guides/introduction/kustomize/)

## Related

- [[17-系统基础/06-知识字典/tooling/helm.md|Helm]]
- [[17-系统基础/06-知识字典/operations/argo.md|Argo]]
- [[17-系统基础/06-知识字典/operations/gitops.md|GitOps]]
- [[17-系统基础/06-知识字典/platform-engineering/manifest.md|Manifest]]
- [[17-系统基础/06-知识字典/workloads/deployment.md|Deployment]]


<!-- risk-assessed -->
