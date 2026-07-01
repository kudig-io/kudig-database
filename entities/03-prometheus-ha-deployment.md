---
title: Prometheus 高可用部署 (entities)
description: '# Prometheus 高可用部署'
category: entities
tags:
- k8s
- cncf
- observability
- 03-prometheus-ha-deployment
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
- Prometheus 高可用部署 是什么
- 如何 Prometheus 高可用部署
trigger_keywords:
- Prometheus
- 高可用部署
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# Prometheus 高可用部署

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

title: Prometheus 高可用部署

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，03-prometheus-ha-deployment 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[atlantis]] — Atlantis
- [[submariner]] — Submariner
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-prometheus-ha-deployment
- [[entities/cncf-observability.md|[[CNCF 可观测性项目全景|CNCF 可观测性项目全景]]]] — Cross-reference
