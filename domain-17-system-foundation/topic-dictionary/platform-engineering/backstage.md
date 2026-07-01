---
title: Backstage
description: Backstage 是 Spotify 开源的开发者门户框架，现为 CNCF 孵化项目。它通过统一的界面集成服务目录、文档、模板和插件，帮助平台工程团队构建内部...
summary: Backstage 是 Spotify 开源的开发者门户框架，现为 CNCF 孵化项目。它通过统一的界面集成服务目录、文档、模板和插件，帮助平台工程团队构建内部...
category: dictionary
tags:
- k8s
- glossary
- backstage
- developer-portal
- platform-engineering
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Backstage 是什么
- Backstage 详解
trigger_keywords:
- Backstage
- dictionary
prerequisites:
- kubectl-basics
---



# Backstage

> **英文名**: Backstage

## 概述

Backstage 是 Spotify 开源的开发者门户框架，现为 CNCF 孵化项目。它通过统一的界面集成服务目录、文档、模板和插件，帮助平台工程团队构建内部开发者体验（IDP）。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Software Catalog | 所有服务和组件的统一目录 |
| Software Templates | 标准化的项目脚手架 |
| TechDocs | 文档即代码（Markdown → 文档站） |
| Plugins | 丰富的插件生态（150+） |
| Search | 跨所有信息的统一搜索 |

### 架构

Backstage 是 React + Node.js 应用，通过 Plugin 架构扩展功能。K8s 插件可展示集群中服务的实时状态。

## 关键机制或特性

- **Service Catalog**：以 YAML 描述每个服务的元数据和所有者。
- **Scaffolder**：一键创建新服务（基于模板）。
- **Kubernetes Plugin**：在 Portal 中查看 Pod/Deployment 状态。
- **API 文档**：自动生成 OpenAPI/gRPC 文档。
- **Scorecards**：服务质量和安全合规评分。

## 使用场景与最佳实践

- 平台团队使用 Backstage 构建内部开发者门户。
- 使用 Software Templates 标准化新服务的创建流程。
- 集成 K8s 插件让开发者在 Portal 中查看服务状态。
- 使用 TechDocs 将服务文档与代码仓库同步。
- 配合 Scorecards 跟踪服务的技术债务和安全合规。

## 参考链接

- [Backstage Official](https://backstage.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane.md|Crossplane]]
- [[domain-17-system-foundation/topic-dictionary/operations/argo.md|Argo]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
