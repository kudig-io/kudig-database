---
title: OpenFunction (entities)
description: '## 概述'
summary: 'OpenFunction 是一个云原生 FaaS (Function as a [[Service|Service]]) 平台，使开发者能够专注于业务逻辑。它集成了 Knative、KEDA、Dapr、Shipwright 等云原生项目，提供从源码构建到函数运行的完整生命周期管理，支持同步和异步函数运行时。'
category: entities
tags:
- k8s
- cncf
- serverless
- openfunction
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFunction 是什么
- 如何 OpenFunction
trigger_keywords:
- OpenFunction
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenFunction

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: Go

## 概述

OpenFunction 是一个云原生 FaaS (Function as a [[Service|Service]]) 平台，使开发者能够专注于业务逻辑。它集成了 Knative、KEDA、Dapr、Shipwright 等云原生项目，提供从源码构建到函数运行的完整生命周期管理，支持同步和异步函数运行时。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **运行时选择**: HTTP API 使用同步函数 (Knative)，消息处理使用异步函数 (KEDA+Dapr)
- **构建缓存**: 配置 [[Buildpacks|BuildPacks]] 缓存加速重复构建
- **资源限制**: 为函数设置合理的 CPU/内存限制，避免资源争抢
- **冷启动优化**: 对延迟敏感的函数设置 `minReplicas: 1` 避免冷启动
- **事件去重**: 异步函数实现幂等处理，应对消息重复投递
- **监控告警**: 监控函数错误率和延迟，配置 KEDA 的 fallback 策略

## 架构定位

在 CNCF 生态中，openfunction 属于 **Serverless** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[knative]] — Knative
- [[keda]] — KEDA
- [[shipwright]] — Shipwright
- [[dapr]] — Dapr
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfunction
- [[entities/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
