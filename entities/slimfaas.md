---
title: SlimFaas (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- serverless
- slimfaas
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
- SlimFaas 是什么
- 如何 SlimFaas
trigger_keywords:
- SlimFaas
prerequisites:
- kubectl-basics
---



# SlimFaas

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: C#

## 概述

SlimFaas 是一个轻量级的 Kubernetes 原生 Function-as-a-[[Service|Service]] (FaaS) 平台，专注于简单性和低资源占用。它可以将普通的 Kubernetes Deployment 作为函数运行，支持 Scale-to-Zero（缩容到零）和按需自动扩容，无需复杂的 FaaS 框架。SlimFaas 通过简单的 HTTP 代理机制转发请求到目标函数，并管理函数的...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **健康检查**: 为函数配置 readinessProbe，确保冷启动后流量正确路由
- **超时配置**: 根据函数的冷启动时间合理设置 scale-down-timeout
- **预热机制**: 对延迟敏感的函数使用 wake-function API 提前预热
- **异步模式**: 耗时操作使用 async-function 模式避免请求超时
- **资源限制**: 为函数设置合理的 CPU/Memory limits 保护集群

## 架构定位

在 CNCF 生态中，slimfaas 属于 **Serverless** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]

## Related

- [[k8up]] — K8up
- [[parsec]] — Parsec
- [[opencost]] — OpenCost
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- slimfaas
- [[entities/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
