---
title: Thanos [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- thanos
- prometheus
- grafana
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Thanos 是什么
- 如何 Thanos
trigger_keywords:
- Thanos
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Thanos

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，thanos 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/observability-pillars.md|observability-pillars]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[submariner]] — Submariner
- [[32-发布/package/2026-07-02_18-40/corpus/core/entities/05-prometheus-ha-deployment]] — [[Prometheus|Prometheus]]us 高可用部署|Prometheus 高可用部署]]
- [[inclavare-containers]] — Inclavare Containers
- [[bank-vaults]] — Bank-Vaults
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 04-thanos-enterprise-metrics-federation
- thanos
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.28
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.31
- [[entities/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[entities/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[concepts/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[skills/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
