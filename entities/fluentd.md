---
title: Fluentd (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- fluentd
- containerd
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
- Fluentd 是什么
- 如何 Fluentd
trigger_keywords:
- Fluentd
prerequisites:
- kubectl-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Fluentd

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Ruby, C

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，fluentd 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/observability-pillars.md|observability-pillars]]

## Related

- [[06-containerd-observability]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-fluentd-enterprise-log-processing
- fluentd
- [[entities/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[concepts/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[skills/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[skills/deployment-workload-selection.md|工作负载控制器选型]] — Cross-reference
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
