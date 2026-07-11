---
title: Radius (entities)
description: 'summary: "Radius 是由 Microsoft 开发的云原生应用平台，提供了一种以应用为中心的方式来定义和部署云原生应用及其依赖的基础设施。'
summary: 'Radius 是由 Microsoft 开发的云原生应用平台，提供了一种以应用为中心的方式来定义和部署云原生应用及其依赖的基础设施。它引入了 Application Graph 的概念，让开发者定义应用需要什么（如数据库、消息队列），而由平台工程师定义如何提供这些资源（Azure CosmosDB 还是本地 MongoDB），实现关注点分离。'
category: entities
tags:
- k8s
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Radius 是什么
- 如何 Radius
trigger_keywords:
- Radius
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Radius

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

Radius 是由 Microsoft 开发的云原生应用平台，2023 年进入 CNCF Sandbox。它的核心理念是 **Application Graph（应用图）**——开发者以应用为中心声明"我的应用需要什么"（如一个 Redis 缓存、一个 PostgreSQL 数据库、一个消息队列），而**平台工程师**定义"如何提供这些资源"（dev 环境用本地 Redis、prod 环境用 Azure Cache for Redis）。这种**关注点分离**让开发者聚焦业务逻辑，平台团队统一管理基础设施。

Radius 通过 **Portable Resource** 抽象基础设施依赖。每个 Portable Resource 对应一种资源类型（`redis`、`mongodb`、`dapr-statestore`），其具体实现由 **Recipe**（基础设施模板）决定。Recipe 可以是 Bicep、Terraform 或 Pulumi 模板，由平台团队维护。这实现了应用在 dev/staging/prod 环境间的无缝迁移——同一应用定义，不同环境使用不同的 Recipe 实现。

## Key Features

- **Application Graph**：以应用为中心，定义应用组件及其资源依赖关系
- **Portable Resource**：与平台无关的资源抽象（如 `redis`、`mongodb`），实现可移植性
- **Recipe 模板**：平台团队维护的 Bicep/Terraform 模板，定义资源的具体实现方式
- **多环境管理**：通过 `Environment` CRD 定义不同环境的 Recipe 集合
- **OCI 分发**：Recipe 作为 OCI Artifact 分发，确保一致性
- **CLI + Dashboard**：`rad` CLI 命令行工具和 Web Dashboard 可视化应用图

## Architecture

Radius 由 **Radius Control Plane**（管理 Application、Environment、Recipe CRD）、**Radius CLI**（`rad` 命令行工具，开发者入口）、**Recipe Engine**（根据 Environment 选择 Recipe 并执行）和 **Dashboard**（可视化 Application Graph）组成。开发者通过 `rad.yaml` 定义应用和资源需求，Radius Controller 根据当前 Environment 匹配对应的 Recipe（如 Terraform 模板），执行并创建实际的基础设施资源。

## K8s 集成

Radius 完全基于 Kubernetes 构建。Application、Environment、Component 等 CRD 通过 Radius Operator 管理。Radius 使用 Crossplane 或 Terraform 作为底层基础设施供给引擎。应用组件渲染为 K8s Deployment + Service，Portable Resource 渲染为 Crossplane XR 或 Terraform Workspace。

## 生产部署要点

- **关注点分离**：开发者通过 Portable Resource 声明需求，平台团队维护 Recipe
- **环境分级**：为 dev/staging/production 创建不同的 Environment 和 Recipe
- **Recipe 标准化**：将 Recipe 作为 OCI Artifact 管理，确保基础设施配置一致
- **应用图**：利用 Application Graph 可视化和理解应用的依赖关系
- **渐进采纳**：从新应用开始使用 Radius，逐步将已有应用迁移

## 生产场景

1. **多环境可移植应用**：应用定义不变，dev 用本地 Redis、prod 用 Azure Cache
2. **标准化基础设施**：平台团队定义标准 Recipe，开发者通过声明式使用
3. **应用依赖可视化**：Application Graph 清晰展示应用的组件和资源依赖
4. **新项目脚手架**：开发者通过 `rad init` 快速创建标准化的云原生应用

## 安装

```bash
# 安装 rad CLI
brew install rad-cli
# 或
curl -fsSL https://raw.githubusercontent.com/radius-project/radius/main/install.sh | bash

# 初始化 Radius 到 K8s 集群
rad install kubernetes

# 创建 Environment
rad env create dev --namespace default
rad env set dev

# 创建 Recipe（平台团队）
rad recipe register redis \
  --env dev \
  --template-kind terraform \
  --template-path "ghcr.io/radius-project/recipes/local-dev/redis"

# 部署应用
cat > app.bicep <<EOF
import radius as radius

resource app 'Applications.Core/applications@2023-10-01-preview' = {
  name: 'myapp'
  properties: {
    environment: 'dev'
  }
}

resource web 'Applications.Core/containers@2023-10-01-preview' = {
  name: 'web'
  properties: {
    application: app.name
    container: { image: 'nginx:latest' }
  }
}
EOF
rad deploy app.bicep
```

## 对比

| 特性 | Radius | Crossplane | Score | KubeVela |
|------|--------|-----------|-------|---------|
| 应用中心 | ✅ App Graph | ❌ 基础设施 | ✅ | ✅ |
| Recipe/模板 | ✅ Bicep/TF | ✅ XR | ❌ | ✅ CUE |
| 多云 | ✅ | ✅ | ❌ | ⚠️ |
| 关注点分离 | ✅ 核心 | ⚠️ | ✅ | ⚠️ |

## 参考链接

- [[crossplane]]
- [[secrets-management]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
