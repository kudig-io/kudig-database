---
title: Konveyor 应用现代化
description: Konveyor 是 Red Hat 开源的 CNCF Sandbox 项目，为应用现代化和迁移提供工具链，包括应用评估、代码分析和迁移规划，帮助企业将传统应用...
summary: Konveyor 是 Red Hat 开源的 CNCF Sandbox 项目，为应用现代化和迁移提供工具链，包括应用评估、代码分析和迁移规划，帮助企业将传统应用...
category: dictionary
tags:
- k8s
- glossary
- operations
- migration
- modernization
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Konveyor 应用现代化 是什么
- Konveyor 详解
trigger_keywords:
- Konveyor 应用现代化
- Konveyor
- dictionary
prerequisites:
- kubernetes
---



# Konveyor 应用现代化（Konveyor）

## 概述

Konveyor 是 Red Hat 开源的 CNCF Sandbox 项目，为应用现代化和迁移提供工具链，包括应用评估、代码分析和迁移规划，帮助企业将传统应用迁移到 Kubernetes 和云原生架构。

## 核心概念/原理

- **应用评估**：评估应用的云原生就绪度和迁移复杂度
- **代码分析**：自动扫描代码中的迁移问题
- **CNCF Sandbox**：Red Hat MTA 的开源核心
- **迁移规划**：生成详细的迁移路径和优先级

## 关键机制或特性

- Tackle（Hub）：应用清单和迁移项目管理
- Analyzer：基于规则的代码静态分析
- Pathfinder：应用评估和风险评估
- Move2Kube：自动化迁移工具
- 丰富的规则集（Java/Spring/Jakarta EE 等）
- 与 AI 集成的迁移建议

## 使用场景与最佳实践

- 传统 Java/Spring 应用到 K8s 的迁移
- 应用组合分析和迁移优先级排序
- 代码级别的迁移问题检测
- 大规模应用现代化项目
- 从 VM/传统部署到容器的转换

## 参考链接

- https://konveyor.io/
- https://github.com/konveyor/konveyor

## Related

- [[系统基础/知识字典/tooling/buildpacks.md|Buildpacks]]
- [[系统基础/知识字典/platform-engineering/backstage.md|Backstage]]
- [[系统基础/知识字典/operations/k8sgpt.md|K8sGPT]]
