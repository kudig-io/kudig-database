---
title: Drasi (entities)
description: '## 概述'
summary: 'Drasi 是由 Microsoft 开发的数据变更处理平台，允许你持续检测数据源中的变更并自动做出反应。它使用 Continuous Query（持续查询）对来自数据库、消息队列、事件流等多种数据源的变更进行实时过滤、聚合和关联，当查询结果发生变化时触发下游动作（如发送通知、调用 API、更新其他系统）。'
category: entities
tags:
- k8s
- cncf
- streaming
- drasi
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Drasi 是什么
- 如何 Drasi
trigger_keywords:
- Drasi
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Drasi

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Rust, C#

## 概述

Drasi 是一个 CNCF 沙箱项目，由 Microsoft 主导开发，是一个分布式数据变更事件处理平台。它监控各种数据源（数据库、API、文件系统）的数据变更，并通过 Reaction 框架触发相应的处理逻辑。Drasi 特别适合 Kubernetes 环境中的 GitOps 场景——监控 Git 仓库、K8s API Server、配置存储的数据变更，并实时驱动配置同步、自动化部署等响应式工作流。

## Key Features（核心能力）

- **数据源监控**：支持 K8s API、Git、Azure CosmosDB、SQL Server 等数据源变更监控
- **Reaction 框架**：通过可插拔的 Reaction 处理数据变更事件
- **查询语言**：使用 Cypher 查询语言定义数据变更关注点
- **边缘部署**：支持在边缘节点部署轻量级 Source
- **低延迟**：基于 Rust 实现的核心引擎，提供毫秒级变更检测
- **K8s 原生**：以 CRD 方式定义 Source、Reaction 和 Query

## 架构与工作原理

Drasi 架构包含三个核心概念：Source 监控数据源变更（如 K8s API Watch、Git Poll）；Query 使用 Cypher 表达式定义关注的数据变更模式；Reaction 是变更事件的处理器，如触发 K8s 部署、发送通知、更新数据库。Drasi Controller 管理这些 CRD 资源的生命周期，Query Engine 基于 Rust 实现高性能的事件匹配和分发。

## K8s 集成

Drasi 以 Kubernetes CRD 形式部署：Source CRD 定义数据源连接和监控规则；Query CRD 定义 Cypher 查询表达式；Reaction CRD 定义事件处理动作。Drasi Controller 监听这些 CRD 的变化并协调相应的处理组件。典型用法是监控 K8s ConfigMap/Secret 变更，自动触发边缘节点的配置同步。

## 生产用例

- **GitOps 自动化**：监控 Git 仓库变更自动触发部署
- **边缘配置同步**：实时同步中心集群配置到边缘节点
- **数据变更通知**：数据库变更事件驱动下游处理
- **K8s 事件响应**：监控 K8s 资源变更并触发自动化工作流

## 安装与快速开始

```bash
kubectl apply -f https://github.com/drasi-project/drasi-platform/releases/latest/download/drasi.yaml
```

## 对比替代方案

相比 ArgoCD/Flux（专注于 GitOps 同步），Drasi 提供更通用的数据变更监控和处理能力。相比 Knative Eventing，Drasi 更专注于声明式的数据源监控。

## Related

- [[youki]] — youki
- [[easegress]] — Easegress
- [[perses]] — Perses
- [[tremor]] — Tremor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- drasi
- [[实体/nats.md|[[NATS|NATS]]]]
- [[实体/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference


<!-- risk-assessed -->
