---
title: Score 工作负载规范
description: 'Score 是 CNCF Sandbox 项目，定义了一个平台无关的工作负载描述规范（score.yaml），开发者只需编写一次工作负载描述，即可通过 Scor...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- workload
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Score 工作负载规范 是什么
- Score 详解
trigger_keywords:
- Score 工作负载规范
- Score
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Score 工作负载规范（Score）

## 概述

Score 是 CNCF Sandbox 项目，定义了一个平台无关的工作负载描述规范（score.yaml），开发者只需编写一次工作负载描述，即可通过 Score CLI 转换为 Kubernetes、Docker Compose、Helm 等平台的具体配置。

## 核心概念/原理

- **平台无关**：一份 score.yaml 描述工作负载需求
- **多目标**：转换为 K8s YAML、Docker Compose、Helm Chart 等
- **开发者友好**：隐藏平台复杂性，专注工作负载需求
- **CNCF Sandbox**：Humanitec 主导

## 关键机制或特性

- `score.yaml` 声明式工作负载描述
- score-compose：转换为 Docker Compose
- score-k8s：转换为 Kubernetes manifests
- score-helm：生成 Helm Chart
- Resource 声明（数据库/缓存/消息队列等依赖）
- score-spec：转换为 Score 内部规范

## 使用场景与最佳实践

- 开发者自助服务平台的底层规范
- 多环境（dev/staging/prod）的配置一致性
- 降低开发者对 K8s 的认知负担
- 平台团队标准化工作负载定义
- IDP（Internal Developer Platform）的工作负载模型

## 参考链接

- https://score.dev/
- https://github.com/score-spec/spec

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/backstage.md|Backstage]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane.md|Crossplane]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
