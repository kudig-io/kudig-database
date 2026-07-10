---
title: kpt 包管理工具
description: kpt 是 Google 开源的 Kubernetes 包管理工具，基于 Git 仓库管理 K8s 配置包，支持包的获取、定制和自动更新，是
  Helm/Kust...
summary: kpt 是 Google 开源的 Kubernetes 包管理工具，基于 Git 仓库管理 K8s 配置包，支持包的获取、定制和自动更新，是 Helm/Kust...
category: dictionary
tags:
- k8s
- glossary
- tooling
- package
- configuration
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kpt 包管理工具 是什么
- kpt 详解
trigger_keywords:
- kpt 包管理工具
- kpt
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kpt 包管理工具（kpt）

## 概述

kpt 是 Google 开源的 Kubernetes 包管理工具，基于 Git 仓库管理 K8s 配置包，支持包的获取、定制和自动更新，是 Helm/Kustomize 之外的配置管理方案。

## 核心概念/原理

- **Git 原生**：以 Git 仓库作为包的存储和分发机制
- **声明式定制**：通过 KRM Function 管道化配置转换
- **自动更新**：上游包更新可自动合并到下游定制
- **Google 开源**：Config Sync 的底层工具

## 关键机制或特性

- `kpt pkg get` 从 Git 获取配置包
- `kpt fn render` 执行 KRM Function 管道
- `kpt live apply` 声明式应用到集群
- KRM Function 生态（Starlark/Go/Container）
- Package 层级和子包管理
- 与 Config Sync / Argo CD 集成

## 使用场景与最佳实践

- GitOps 配置的管理和分发
- 多环境配置的包化管理
- 配置模板的版本控制和更新
- KRM Function 的声明式配置转换
- 大规模 K8s 配置的组织和管理

## 参考链接

- https://kpt.dev/
- https://github.com/GoogleContainerTools/kpt

## Related

- [[系统基础/知识字典/tooling/helm.md|Helm]]
- [[系统基础/知识字典/tooling/kustomize.md|Kustomize]]
- [[系统基础/知识字典/operations/flux.md|Flux]]


<!-- risk-assessed -->
