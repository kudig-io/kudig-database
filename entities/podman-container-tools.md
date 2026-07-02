---
title: Podman Desktop [entities]
description: '## 概述'
summary: 'Podman Desktop 是一个开源的桌面容器管理工具，为开发者提供图形化界面来管理容器、镜像、Pod 和 Kubernetes 集群。它支持 Podman、Docker 和 Kubernetes 等多种容器引擎，让开发者可以在本地无缝地开发、测试和调试容器化应用，并轻松迁移到 Kubernetes 环境。'
category: entities
tags:
- k8s
- cncf
- runtime
- podman-container-tools
- containerd
- docker
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Podman Desktop 是什么
- 如何 Podman Desktop
trigger_keywords:
- Podman
- Desktop
prerequisites:
- kubectl-basics
---



# Podman Desktop

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: TypeScript

## 概述

Podman Desktop 是一个开源的桌面容器管理工具，为开发者提供图形化界面来管理容器、镜像、Pod 和 Kubernetes 集群。它支持 Podman、Docker 和 Kubernetes 等多种容器引擎，让开发者可以在本地无缝地开发、测试和调试容器化应用，并轻松迁移到 Kubernetes 环境。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Rootless 优先**: 使用 Podman 的 rootless 模式提高安全性
- **资源限制**: 为 Podman Machine 配置合适的 CPU 和内存
- **镜像清理**: 定期清理未使用的镜像和卷
- **本地 K8s**: 使用 KIND 快速创建一次性测试集群
- **扩展生态**: 探索扩展目录，增强开发体验

## 架构定位

在 CNCF 生态中，podman-container-tools 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[deployment]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[devspace]] — DevSpace
- [[openfeature]] — OpenFeature
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[podman-desktop]] — Podman Desktop

- podman-container-tools
- [[entities/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
