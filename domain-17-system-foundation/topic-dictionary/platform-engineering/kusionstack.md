---
title: Kusion 配置管理
description: KusionStack 是蚂蚁集团开源的 CNCF Sandbox 项目，面向应用的配置管理平台，使用 KCL 语言定义应用配置，整合 Kubernetes/T...
summary: KusionStack 是蚂蚁集团开源的 CNCF Sandbox 项目，面向应用的配置管理平台，使用 KCL 语言定义应用配置，整合 Kubernetes/T...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- configuration
- iac
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kusion 配置管理 是什么
- KusionStack 详解
trigger_keywords:
- Kusion 配置管理
- KusionStack
- dictionary
prerequisites:
- kubernetes
---



# Kusion 配置管理（KusionStack）

## 概述

KusionStack 是蚂蚁集团开源的 CNCF Sandbox 项目，面向应用的配置管理平台，使用 KCL 语言定义应用配置，整合 Kubernetes/Terraform/云资源为统一的应用交付。

## 核心概念/原理

- **应用配置管理**：面向应用的统一配置定义
- **KCL 语言**：类型安全的配置语言
- **CNCF Sandbox**：蚂蚁集团主导
- **多后端**：K8s/Terraform/云 API 统一交付

## 关键机制或特性

- AppConfiguration 模型定义应用
- KCL 语言编写配置
- Module 可复用配置模块
- Workspace 多环境管理
- 预览（Preview）变更影响
- 与 Kubernetes/Terraform 集成
- Kusion API Server

## 使用场景与最佳实践

- 企业内部的应用配置标准化
- 多环境（dev/staging/prod）配置管理
- IaC 的编程化管理
- 开发者自助的应用交付
- 复杂应用的声明式定义

## 参考链接

- https://kusionstack.io/
- https://github.com/kusionstack/kusion

## Related

- [[domain-17-system-foundation/topic-dictionary/configuration/kcl.md|KCL]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane.md|Crossplane]]
