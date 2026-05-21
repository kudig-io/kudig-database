---
title: Thanos
description: '## 概述'
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
- [[03-prometheus-ha-deployment]] — Prometheus 高可用部署
- [[inclavare-containers]] — Inclavare Containers
- [[bank-vaults]] — Bank-Vaults
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-06-observability/04-thanos-enterprise-metrics-federation.md|04-thanos-enterprise-metrics-federation]]
- [[domain-19-landscape-references/incubating/thanos/thanos.md|thanos]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.26.md|RELEASE-NOTES-0.26]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.36.md|RELEASE-NOTES-0.36]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.22.md|RELEASE-NOTES-0.22]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.32.md|RELEASE-NOTES-0.32]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.23.md|RELEASE-NOTES-0.23]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.33.md|RELEASE-NOTES-0.33]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.27.md|RELEASE-NOTES-0.27]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.37.md|RELEASE-NOTES-0.37]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.28.md|RELEASE-NOTES-0.28]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.38.md|RELEASE-NOTES-0.38]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.29.md|RELEASE-NOTES-0.29]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.39.md|RELEASE-NOTES-0.39]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.30.md|RELEASE-NOTES-0.30]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.41.md|RELEASE-NOTES-0.41]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.24.md|RELEASE-NOTES-0.24]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.34.md|RELEASE-NOTES-0.34]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.40.md|RELEASE-NOTES-0.40]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.25.md|RELEASE-NOTES-0.25]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.35.md|RELEASE-NOTES-0.35]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.21.md|RELEASE-NOTES-0.21]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.31.md|RELEASE-NOTES-0.31]]
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[references/k8s-observability-ecosystem|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[skills/monitoring-fta|监控与告警异常故障树分析]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
