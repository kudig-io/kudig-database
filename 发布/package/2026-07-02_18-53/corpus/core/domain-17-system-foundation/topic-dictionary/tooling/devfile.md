---
title: Devfile 开发环境规范
description: Devfile 是 CNCF Sandbox 项目，定义了云开发环境的声明式规范，用 YAML 描述开发环境的组件、命令和依赖，实现开发环境的可移植和可复现。...
summary: Devfile 是 CNCF Sandbox 项目，定义了云开发环境的声明式规范，用 YAML 描述开发环境的组件、命令和依赖，实现开发环境的可移植和可复现。...
category: dictionary
tags:
- k8s
- glossary
- tooling
- development
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Devfile 开发环境规范 是什么
- Devfile 详解
trigger_keywords:
- Devfile 开发环境规范
- Devfile
- dictionary
prerequisites:
- kubernetes
---



# Devfile 开发环境规范（Devfile）

## 概述

Devfile 是 CNCF Sandbox 项目，定义了云开发环境的声明式规范，用 YAML 描述开发环境的组件、命令和依赖，实现开发环境的可移植和可复现。

## 核心概念/原理

- **开发环境标准**：统一描述开发环境的 YAML 规范
- **可移植**：同一 Devfile 可在多种平台运行
- **CNCF Sandbox**：Red Hat/OpenShift Dev Spaces 核心
- **Registry**：社区 Devfile 仓库

## 关键机制或特性

- devfile.yaml 定义开发环境
- Components（容器/Volume/Git 组件）
- Commands（build/run/test/debug）
- 预置开发栈（Java/Node.js/Go/Python 等）
- Devfile Registry 社区仓库
- DevWorkspace Operator K8s 集成

## 使用场景与最佳实践

- 团队开发环境的标准化
- 云端 IDE 的开发环境配置
- 新成员的快速环境搭建
- CI/CD 中的开发环境复现
- 多平台开发环境的统一管理

## 参考链接

- https://devfile.io/
- https://github.com/devfile/api

## Related

- [[domain-17-system-foundation/知识字典/tooling/telepresence.md|Telepresence]]
- [[domain-17-system-foundation/知识字典/tooling/minikube.md|Minikube]]
- [[domain-17-system-foundation/知识字典/platform-engineering/backstage.md|Backstage]]
