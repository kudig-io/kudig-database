---
title: SlimToolkit (entities)
description: '## 概述'
summary: 'SlimToolkit（原名 DockerSlim）是一个容器镜像优化工具，能够自动分析和瘦身容器镜像，将镜像大小缩减高达 30 倍，同时提升安全性。它通过动态分析识别应用实际需要的文件，移除不必要的组件，生成最小化、安全加固的生产镜像。'
category: entities
tags:
- k8s
- cncf
- image
- slimtoolkit
- docker
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SlimToolkit 是什么
- 如何 SlimToolkit
trigger_keywords:
- SlimToolkit
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SlimToolkit

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

SlimToolkit（原名 DockerSlim）是一个容器镜像优化工具，能够自动分析和瘦身容器镜像，将镜像大小缩减高达 30 倍，同时提升安全性。它通过动态分析识别应用实际需要的文件，移除不必要的组件，生成最小化、安全加固的生产镜像。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，slimtoolkit 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[linkerd]] — Linkerd
- [[oscal-compass]] — [[entities/oscal-compass.md|OSCAL Compass]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- slimtoolkit
- [[entities/zot.md|zot]]
- [[entities/eraser.md|Eraser]]
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
