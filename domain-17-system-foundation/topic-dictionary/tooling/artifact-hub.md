---
title: Artifact Hub 制品市场
description: 'Artifact Hub 是 CNCF 孵化项目，云原生制品的集中发现和分发平台，支持 Helm Chart、OPA 策略、OOCI 镜像、Kustomize、...'
category: dictionary
tags:
- k8s
- glossary
- tooling
- registry
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Artifact Hub 制品市场 是什么
- Artifact Hub 详解
trigger_keywords:
- Artifact Hub 制品市场
- Artifact Hub
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Artifact Hub 制品市场（Artifact Hub）

## 概述

Artifact Hub 是 CNCF 孵化项目，云原生制品的集中发现和分发平台，支持 Helm Chart、OPA 策略、OOCI 镜像、Kustomize、Tekton 等多种云原生制品的搜索和发布。

## 核心概念/原理

- **多制品类型**：Helm/OPA/Tekton/Kustomize/Keptn/CoreDNS 等
- **CNCF 孵化**：云原生制品的标准市场
- **搜索发现**：统一的搜索和元数据索引
- **社区驱动**：开放的制品发布平台

## 关键机制或特性

- 支持 Helm Chart、Container、OPA、Tinkerbell、Keda 等制品
- 仓库管理和版本控制
- 安全评分（基于 Trivy/Grype 扫描）
- 用户评价和收藏
- Star 和 Fork 机制
- Webhook 通知
- CLI 工具 `ah` 管理

## 使用场景与最佳实践

- 云原生制品的发现和搜索
- Helm Chart 的发布和分发
- OPA 策略库的共享
- CI/CD 模板的市场
- 安全评估的参考平台

## 参考链接

- https://artifacthub.io/
- https://github.com/artifacthub/hub

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/distribution.md|Distribution]]
- [[domain-17-system-foundation/topic-dictionary/tooling/harbor.md|Harbor]]
