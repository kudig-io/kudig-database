---
title: Perses 云原生仪表盘
description: Perses 是 CNCF Sandbox 项目，云原生可观测性仪表盘工具，旨在提供 GitOps 友好的仪表盘管理方式，支持声明式定义仪表盘并通过
  Git 进...
summary: Perses 是 CNCF Sandbox 项目，云原生可观测性仪表盘工具，旨在提供 GitOps 友好的仪表盘管理方式，支持声明式定义仪表盘并通过
  Git 进...
category: dictionary
tags:
- k8s
- glossary
- observability
- dashboard
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
- Perses 云原生仪表盘 是什么
- Perses 详解
trigger_keywords:
- Perses 云原生仪表盘
- Perses
- dictionary
prerequisites:
- kubernetes
---



# Perses 云原生仪表盘（Perses）

## 概述

Perses 是 CNCF Sandbox 项目，云原生可观测性仪表盘工具，旨在提供 GitOps 友好的仪表盘管理方式，支持声明式定义仪表盘并通过 Git 进行版本管理。

## 核心概念/原理

- **GitOps 仪表盘**：声明式 YAML 定义仪表盘，通过 Git 管理
- **Prometheus 优先**：深度集成 Prometheus/Thanos 数据源
- **CNCF Sandbox**：Grafana 的声明式替代方案
- **可扩展**：插件式面板和主题

## 关键机制或特性

- Dashboard CRD 声明式仪表盘
- Datasource CRD 数据源管理
- 支持 Prometheus/Thanos/Cortex 数据源
- 变量（Variables）和模板系统
- 面板（Panels）插件生态
- Perses CLI 和 Web UI
- 与 Perses Operator 集成

## 使用场景与最佳实践

- GitOps 方式的仪表盘管理
- Grafana 的声明式替代
- 多环境仪表盘的一致性管理
- 可观测性平台的标准仪表盘
- 仪表盘代码审查和版本控制

## 参考链接

- https://perses.dev/
- https://github.com/perses/perses

## Related

- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]
- [[系统基础/知识字典/observability/thanos.md|Thanos]]
- [[系统基础/知识字典/observability/grafana.md|Grafana]]
