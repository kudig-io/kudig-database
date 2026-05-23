---
title: Devfile [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- ci-cd
- devfile
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Devfile 是什么
- 如何 Devfile
trigger_keywords:
- Devfile
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Devfile

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Devfile 是一个开放标准，用于定义云原生开发环境。它通过 YAML 格式的 devfile.yaml 描述开发工具容器、端口转发、命令和生命周期事件，使开发环境可复现、可共享，并被多种 IDE 和开发工具支持（如 Eclipse Che、odo、OpenShift Dev Spaces）。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **仓库内置**: 将 devfile.yaml 放在项目根目录，确保所有开发者环境一致
- **Registry 复用**: 使用 Devfile Registry 提供的模板作为 parent
- **命令分组**: 将命令按 build/run/test/debug 分组，便于 IDE 集成
- **资源限制**: 为容器设置合理的 CPU 和内存限制
- **环境变量**: 使用 env 配置开发环境变量，避免硬编码

## 架构定位

在 CNCF 生态中，devfile 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/kubernetes-architecture-overview|kubernetes-architecture-overview]]

## Related

- [[entities/external-secrets|secrets]]]] — External Secrets Operator
- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- devfile
- [[entities/shipwright|Shipwright]]
- [[entities/atlantis|Atlantis]]
- [[entities/dalec|Dalec]]
- [[entities/werf|werf]]
- [[entities/pipecd|PipeCD]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
