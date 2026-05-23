---
title: Fluentd (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- fluentd
- containerd
- crd
- operator
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
created: "2026-05-23"
---

# Fluentd

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Ruby, C

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，fluentd 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/observability-pillars|observability-pillars]]

## Related

- [[06-containerd-observability]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-fluentd-enterprise-log-processing
- fluentd
- [[references/k8s-observability-ecosystem|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[skills/k8s-logging-management-guide|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[skills/deployment-workload-selection|工作负载控制器选型]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
