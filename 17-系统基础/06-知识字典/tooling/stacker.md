---
title: Stacker 容器构建
description: Stacker 是 Project Atomic（Red Hat）开源的容器镜像构建工具，使用声明式 YAML 定义构建步骤，支持层缓存和
  OCI 格式输出，是...
summary: Stacker 是 Project Atomic（Red Hat）开源的容器镜像构建工具，使用声明式 YAML 定义构建步骤，支持层缓存和 OCI
  格式输出，是...
category: dictionary
tags:
- k8s
- glossary
- tooling
- container
- build
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Stacker 容器构建 是什么
- Stacker 详解
trigger_keywords:
- Stacker 容器构建
- Stacker
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Stacker 容器构建（Stacker）

## 概述

Stacker 是 Project Atomic（Red Hat）开源的容器镜像构建工具，使用声明式 YAML 定义构建步骤，支持层缓存和 OCI 格式输出，是 Buildah/Kaniko 之外的容器构建方案。

## 核心概念/原理

- **声明式构建**：YAML 定义镜像构建步骤
- **层缓存**：智能缓存未变更的层
- **OCI 输出**：生成标准 OCI 镜像
- **无 Daemon**：无需 Docker Daemon 即可构建

## 关键机制或特性

- stacker.yaml 定义构建流程
- 支持从基础镜像/Dockerfile/OCI 开始
- 层绑定（bind）和导入（import）
- 构建参数化
- 多阶段构建支持
- 签名和推送

## 使用场景与最佳实践

- CI/CD Pipeline 的容器构建
- 无 Docker Daemon 环境的镜像构建
- 声明式镜像定义
- 层缓存优化的构建流程
- Buildah/Kaniko 的替代方案

## 参考链接

- https://stackerbuild.io/
- https://github.com/project-stacker/stacker

## Related

- [[17-系统基础/06-知识字典/tooling/buildpacks.md|Buildpacks]]
- [[17-系统基础/06-知识字典/tooling/shipwright.md|Shipwright]]
- [[17-系统基础/06-知识字典/fundamentals/docker.md|Docker]]


<!-- risk-assessed -->
