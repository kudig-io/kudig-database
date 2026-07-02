---
title: DevSpace 云开发环境
description: DevSpace 是 Loft Labs 开源的云原生开发工具，为 Kubernetes 提供一键式开发环境搭建、实时同步和热重载，简化
  K8s 上的开发工作流...
summary: DevSpace 是 Loft Labs 开源的云原生开发工具，为 Kubernetes 提供一键式开发环境搭建、实时同步和热重载，简化 K8s
  上的开发工作流...
category: dictionary
tags:
- k8s
- glossary
- tooling
- development
- k8s
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DevSpace 云开发环境 是什么
- DevSpace 详解
trigger_keywords:
- DevSpace 云开发环境
- DevSpace
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DevSpace 云开发环境（DevSpace）

## 概述

DevSpace 是 Loft Labs 开源的云原生开发工具，为 Kubernetes 提供一键式开发环境搭建、实时同步和热重载，简化 K8s 上的开发工作流。

## 核心概念/原理

- **一键环境**：`devspace dev` 一键进入 K8s 开发环境
- **实时同步**：文件变更实时同步到容器
- **Loft Labs**：vcluster 团队出品
- **DevContainer 兼容**：支持 devcontainer.json

## 关键机制或特性

- devspace.yaml 声明式开发环境配置
- 文件双向同步（rsync 式）
- 端口转发自动配置
- 终端代理（直接在 K8s Pod 中执行命令）
- Helm/Kubectl 部署集成
- 多服务并行开发
- Plugin 扩展

## 使用场景与最佳实践

- K8s 微服务的本地开发
- 多服务联调环境
- 替代 Telepresence 的开发方案
- 团队的标准化开发环境
- 开发/测试环境的快速搭建

## 参考链接

- https://devspace.sh/
- https://github.com/loft-sh/devspace

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/telepresence.md|Telepresence]]
- [[domain-17-system-foundation/topic-dictionary/tooling/skaffold.md|Skaffold]]
- [[domain-17-system-foundation/topic-dictionary/tooling/devfile.md|Devfile]]


<!-- risk-assessed -->
