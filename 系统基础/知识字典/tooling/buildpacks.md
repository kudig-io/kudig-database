---
title: Cloud Native Buildpacks
description: Cloud Native Buildpacks（CNB）是 CNCF 孵化项目，将应用源代码自动转化为容器镜像，无需编写 Dockerfile，支持多语言和多框...
summary: Cloud Native Buildpacks（CNB）是 CNCF 孵化项目，将应用源代码自动转化为容器镜像，无需编写 Dockerfile，支持多语言和多框...
category: dictionary
tags:
- k8s
- glossary
- tooling
- ci-cd
- container
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Native Buildpacks 是什么
- CNB 详解
trigger_keywords:
- Cloud Native Buildpacks
- CNB
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cloud Native Buildpacks（CNB）

## 概述

Cloud Native Buildpacks（CNB）是 CNCF 孵化项目，将应用源代码自动转化为容器镜像，无需编写 Dockerfile，支持多语言和多框架，是 Heroku Buildpacks 的云原生演进。

## 核心概念/原理

- **无 Dockerfile**：自动检测语言和框架，生成优化的容器镜像
- **可复现构建**：相同输入产生相同的镜像输出（Reproducible Builds）
- **多阶段优化**：自动分离构建依赖和运行时依赖
- **CNCF 孵化**：VMware/Pivotal 主导，社区活跃

## 关键机制或特性

- Builder 镜像：包含 Detect + Build 阶段的执行环境
- Pack CLI：命令行工具（`pack build`）
- Buildpack 检测顺序和组管理
- 层缓存（Layer Caching）优化构建速度
- Rebase：仅替换基础镜像层，无需重新构建
- Platform API 与 K8s Tekton/Jenkins 集成

## 使用场景与最佳实践

- 无需维护 Dockerfile 的应用容器化
- 多语言 monorepo 的统一构建流程
- 安全补丁的快速应用（Rebase）
- CI/CD Pipeline 中的标准化构建
- PaaS 平台底层的镜像构建引擎

## 参考链接

- https://buildpacks.io/
- https://github.com/buildpacks/pack

## Related

- [[系统基础/topic-dictionary/fundamentals/docker.md|Docker]]
- [[系统基础/topic-dictionary/operations/tekton.md|Tekton]]
- [[系统基础/topic-dictionary/tooling/podman.md|Podman]]


<!-- risk-assessed -->
