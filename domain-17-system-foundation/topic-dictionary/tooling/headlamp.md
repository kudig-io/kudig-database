---
title: Headlamp K8s 仪表盘
description: 'Headlamp 是 Kinvolk（现微软）开源的 Kubernetes 管理仪表盘，提供集群资源可视化、日志查看和终端操作，是 K8s Dashboard ...'
category: dictionary
tags:
- k8s
- glossary
- tooling
- dashboard
- ui
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Headlamp K8s 仪表盘 是什么
- Headlamp 详解
trigger_keywords:
- Headlamp K8s 仪表盘
- Headlamp
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Headlamp K8s 仪表盘（Headlamp）

## 概述

Headlamp 是 Kinvolk（现微软）开源的 Kubernetes 管理仪表盘，提供集群资源可视化、日志查看和终端操作，是 K8s Dashboard 的现代替代方案，支持插件扩展。

## 核心概念/原理

- **现代 UI**：基于 React + TypeScript 的现代 Web 界面
- **插件架构**：可扩展的插件系统
- **多集群**：支持同时管理多个集群
- **Kinvolk 出品**：Flatcar Container Linux 团队开发

## 关键机制或特性

- 集群资源概览（Pods/Services/Deployments 等）
- 实时日志查看和终端 Shell
- YAML 编辑器（在线编辑资源）
- 插件市场（社区和企业插件）
- 多集群管理和切换
- 自定义主题和品牌定制

## 使用场景与最佳实践

- Kubernetes 集群的可视化管理
- 替代 K8s Dashboard 的现代方案
- 开发者的日常集群操作界面
- 运维团队的集群监控仪表盘
- 需要品牌定制的企业管理平台

## 参考链接

- https://headlamp.io/
- https://github.com/headlamp-k8s/headlamp

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/stern.md|Stern]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
