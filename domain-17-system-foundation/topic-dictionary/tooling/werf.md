---
title: werf CI/CD 工具
description: 'werf 是 Flant 开源的 CNCF Sandbox 项目，一站式 CI/CD 工具，集成构建、部署和运维功能，支持 GitOps 工作流，将 Docke...'
category: dictionary
tags:
- k8s
- glossary
- tooling
- ci-cd
- gitops
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- werf CI/CD 工具 是什么
- werf 详解
trigger_keywords:
- werf CI/CD 工具
- werf
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# werf CI/CD 工具（werf）

## 概述

werf 是 Flant 开源的 CNCF Sandbox 项目，一站式 CI/CD 工具，集成构建、部署和运维功能，支持 GitOps 工作流，将 Dockerfile/Helm/K8s 整合为统一的工作流。

## 核心概念/原理

- **一站式**：构建 + 推送 + 部署的完整 CI/CD 流程
- **GitOps 原生**：以 Git 为唯一配置源
- **CNCF Sandbox**：Flant 主导
- **多环境**：支持 dev/staging/prod 环境管理

## 关键机制或特性

- werf.yaml 定义构建和部署
- Stapel/Buildah 构建引擎
- Helm Chart 集成部署
- 三态 Git 重设（Three-stage Git-based rebasing）
- 自动清理过期镜像
- Namespace/Release 管理
- werf converge 一键部署

## 使用场景与最佳实践

- GitOps 的完整 CI/CD Pipeline
- Helm Chart 的自动化部署
- 开发环境的快速搭建
- 多环境的应用管理
- 替代 Argo/Flux 的一站式方案

## 参考链接

- https://werf.io/
- https://github.com/werf/werf

## Related

- [[domain-17-system-foundation/topic-dictionary/operations/argo.md|Argo]]
- [[domain-17-system-foundation/topic-dictionary/operations/flux.md|Flux]]
- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
