---
title: OpenFeature 特性标志
description: OpenFeature 是 CNCF 孵化项目，定义了特性标志（Feature Flags）的通用 API 标准，使应用代码与特性标志提供商解耦，支持
  Laun...
summary: OpenFeature 是 CNCF 孵化项目，定义了特性标志（Feature Flags）的通用 API 标准，使应用代码与特性标志提供商解耦，支持
  Laun...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- feature-flags
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
- OpenFeature 特性标志 是什么
- OpenFeature 详解
trigger_keywords:
- OpenFeature 特性标志
- OpenFeature
- dictionary
prerequisites:
- kubernetes
---



# OpenFeature 特性标志（OpenFeature）

## 概述

OpenFeature 是 CNCF 孵化项目，定义了特性标志（Feature Flags）的通用 API 标准，使应用代码与特性标志提供商解耦，支持 LaunchDarkly/Flagsmith/GO Feature Flag 等多种后端。

## 核心概念/原理

- **API 标准**：统一的特性标志 API（不绑定特定提供商）
- **多后端**：支持 LaunchDarkly/Flagsmith/GO Feature Flag/CloudBees 等
- **CNCF 孵化**：社区驱动的特性标志标准化
- **多语言 SDK**：Go/Java/JavaScript/Python/.NET 等

## 关键机制或特性

- Client API（评估特性标志值）
- Provider 接口（对接不同后端）
- Evaluation Context（用户/环境上下文）
- Hooks（日志/指标/追踪集成）
- Targeting Rules（基于上下文的动态规则）
- OFREP（OpenFeature Remote Evaluation Protocol）

## 使用场景与最佳实践

- 应用中的特性标志管理
- A/B 测试和渐进式发布
- 多提供商的特性标志统一管理
- 开发者自助的特性控制
- 与 CI/CD 集成的发布策略

## 参考链接

- https://openfeature.dev/
- https://github.com/open-feature/spec

## Related

- [[domain-17-system-foundation/知识字典/operations/flagger.md|Flagger]]
- [[domain-17-system-foundation/知识字典/operations/argo.md|Argo]]
- [[domain-17-system-foundation/知识字典/operations/pipecd.md|PipeCD]]
