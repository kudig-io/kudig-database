---
title: Podman Desktop [entities] [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- podman-desktop
- docker
- crd
- operator
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
created: "2026-05-23"
---

# Podman Desktop

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: TypeScript, Svelte

## 概述

Podman Desktop 是一个开源的图形化容器管理工具，为开发者提供在本地管理容器、Pod 和 Kubernetes 的统一桌面体验。它支持 Podman、Docker、Lima 等多种容器引擎，并提供可扩展的插件系统，帮助开发者在 macOS、Windows 和 Linux 上无缝进行云原生开发。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Rootless 模式**: 优先使用 Podman 的 rootless 模式提升安全性
- **资源管理**: 在 Settings 中合理配置 Podman Machine 的 CPU 和内存
- **镜像清理**: 定期使用 `podman system prune` 清理未使用的资源
- **Compose 优先**: 多容器开发使用 Compose 文件管理，便于团队共享
- **Kind 开发**: 使用 Kind 集群进行本地 Kubernetes 开发和测试
- **扩展开发**: 利用扩展 API 自定义开发工作流

## 架构定位

在 CNCF 生态中，podman-desktop 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[pod-lifecycle]]

## Related

- [[openchoreo]] — OpenChoreo
- [[docker]] — Docker
- tools]] — Podman Desktop
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[lima]] — Lima

- podman-desktop
- [[entities/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
