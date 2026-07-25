---
title: Skaffold 开发工具
description: Skaffold 是 Google 开源的 K8s 开发工具，自动化构建/推送/部署的完整循环，支持文件监控、端口转发和调试模式，是 K8s
  开发者的标准效率工...
summary: Skaffold 是 Google 开源的 K8s 开发工具，自动化构建/推送/部署的完整循环，支持文件监控、端口转发和调试模式，是 K8s 开发者的标准效率工...
category: dictionary
tags:
- k8s
- glossary
- tooling
- development
- google
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Skaffold 开发工具 是什么
- Skaffold 详解
trigger_keywords:
- Skaffold 开发工具
- Skaffold
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Skaffold 开发工具（Skaffold）

## 概述

Skaffold 是 Google 开源的 K8s 开发工具，自动化构建/推送/部署的完整循环，支持文件监控、端口转发和调试模式，是 K8s 开发者的标准效率工具。

## 核心概念/原理

- **开发循环**：代码变更 → 构建 → 推送 → 部署的自动化
- **Google 出品**：Cloud Code 的底层引擎
- **多构建器**：Docker/Jib/Buildpacks/Kaniko/ko
- **调试模式**：端口转发和远程调试支持

## 关键机制或特性

- `skaffold dev` 开发模式（文件监控 + 自动重部署）
- `skaffold run` 单次构建部署
- `skaffold debug` 调试模式（端口转发）
- 支持 Docker/Jib/Buildpacks/Kaniko/ko 构建器
- Helm/Kustomize/Kpt/raw YAML 部署
- 多模块（Artifacts）并行构建
- Profile 环境切换（dev/staging/prod）

## 使用场景与最佳实践

- K8s 应用的日常开发循环
- 微服务的联调环境
- CI/CD Pipeline 的本地验证
- 团队的标准化开发工具
- 最佳实践：dev profile + prod profile、build concurrency、port-forward

## 参考链接

- https://skaffold.dev/
- https://github.com/GoogleContainerTools/skaffold

## Related

- [[17-系统基础/06-知识字典/tooling/devspace.md|DevSpace]]
- [[17-系统基础/06-知识字典/tooling/telepresence.md|Telepresence]]
- [[17-系统基础/06-知识字典/tooling/helm.md|Helm]]


<!-- risk-assessed -->
