---
title: Shipwright 容器构建
description: 'Shipwright 是 Red Hat 开源的 CNCF Sandbox 项目，在 Kubernetes 上提供声明式的容器镜像构建框架，支持 Buildpa...'
category: dictionary
tags:
- k8s
- glossary
- tooling
- container
- build
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Shipwright 容器构建 是什么
- Shipwright 详解
trigger_keywords:
- Shipwright 容器构建
- Shipwright
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Shipwright 容器构建（Shipwright）

## 概述

Shipwright 是 Red Hat 开源的 CNCF Sandbox 项目，在 Kubernetes 上提供声明式的容器镜像构建框架，支持 Buildpacks、Buildah、Kaniko 等多种构建策略。

## 核心概念/原理

- **K8s 原生构建**：在集群内以 Pod 方式执行镜像构建
- **多策略**：支持 Buildpacks/Buildah/Kaniko/Dockerfile
- **CNCF Sandbox**：Red Hat/Tekton 生态组件
- **Tekton 集成**：可作为 Tekton Pipeline 的构建步骤

## 关键机制或特性

- Build / BuildRun CRD 定义构建任务
- ClusterBuildStrategy / BuildStrategy 构建策略
- 支持 Dockerfile/Buildpacks/Buildah/Ko 等
- 源码从 Git/Bundle 获取
- 推送到任意 OCI Registry
- Tekton Task 集成
- 构建参数化和模板

## 使用场景与最佳实践

- K8s 集群内的镜像构建
- CI/CD Pipeline 的构建步骤
- 多构建策略的统一框架
- 无 Docker Daemon 的镜像构建
- 企业内部的安全镜像构建

## 参考链接

- https://shipwright.io/
- https://github.com/shipwright-io/build

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/buildpacks.md|Buildpacks]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker.md|Docker]]
- [[domain-17-system-foundation/topic-dictionary/operations/tekton.md|Tekton]]
