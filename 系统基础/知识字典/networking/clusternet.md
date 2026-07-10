---
title: Clusternet 多集群网络
description: Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API
  访问和资源分发，无...
summary: Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API 访问和资源分发，无...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
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
- Clusternet 多集群网络 是什么
- Clusternet 详解
trigger_keywords:
- Clusternet 多集群网络
- Clusternet
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Clusternet 多集群网络（Clusternet）

## 概述

Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API 访问和资源分发，无需修改底层网络。

## 核心概念/原理

- **API 代理**：通过代理方式访问子集群 API，无需直连
- **应用分发**：支持 ManifestWork 式的应用分发
- **Scheduler 插件**：多集群调度策略
- **CNCF Sandbox**：轻量级多集群管理方案

## 关键机制或特性

- Hub 集群 + Agent 部署模式
- ServiceExport / ServiceImport 多集群服务发现
- 跨集群 Helm Chart 安装
- 多集群调度框架插件
- 支持边缘集群（弱网环境）
- 与 Karmada 互补的多集群方案

## 使用场景与最佳实践

- 多集群 API 统一访问
- 跨集群应用分发和管理
- 边缘集群的集中管理
- 弱网环境下的集群互联
- 多集群 Helm 应用编排

## 参考链接

- https://clusternet.io/
- https://github.com/clusternet/clusternet

## Related

- [[系统基础/topic-dictionary/platform-engineering/karmada.md|Karmada]]
- [[系统基础/topic-dictionary/networking/submariner.md|Submariner]]
- [[系统基础/topic-dictionary/platform-engineering/rancher.md|Rancher]]


<!-- risk-assessed -->
