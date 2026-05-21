---
title: PromQL 高级查询
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- 02-prometheus-promql-advanced
- prometheus
- grafana
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- PromQL 高级查询 是什么
- 如何 PromQL 高级查询
trigger_keywords:
- PromQL
- 高级查询
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

# PromQL 高级查询

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

title: Prometheus 高级 PromQL

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，02-prometheus-promql-advanced 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[pod-lifecycle]]

## Related

- [[metallb]] — MetalLB
- [[buildpacks]] — Cloud Native Buildpacks
- [[kube-rs]] — kube-rs
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md|02-prometheus-promql-advanced]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.32.md|RELEASE-NOTES-2.32]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.22.md|RELEASE-NOTES-2.22]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.47.md|RELEASE-NOTES-2.47]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.16.md|RELEASE-NOTES-2.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.36.md|RELEASE-NOTES-2.36]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.53.md|RELEASE-NOTES-2.53]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.12.md|RELEASE-NOTES-2.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.43.md|RELEASE-NOTES-2.43]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.26.md|RELEASE-NOTES-2.26]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.37.md|RELEASE-NOTES-2.37]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.52.md|RELEASE-NOTES-2.52]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.13.md|RELEASE-NOTES-2.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.42.md|RELEASE-NOTES-2.42]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.27.md|RELEASE-NOTES-2.27]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.33.md|RELEASE-NOTES-2.33]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.23.md|RELEASE-NOTES-2.23]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.46.md|RELEASE-NOTES-2.46]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.17.md|RELEASE-NOTES-2.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.4.md|RELEASE-NOTES-2.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.38.md|RELEASE-NOTES-2.38]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.28.md|RELEASE-NOTES-2.28]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.18.md|RELEASE-NOTES-2.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.49.md|RELEASE-NOTES-2.49]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.19.md|RELEASE-NOTES-2.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.48.md|RELEASE-NOTES-2.48]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.5.md|RELEASE-NOTES-2.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.39.md|RELEASE-NOTES-2.39]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.29.md|RELEASE-NOTES-2.29]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.6.md|RELEASE-NOTES-2.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.7.md|RELEASE-NOTES-3.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.10.md|RELEASE-NOTES-3.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.11.md|RELEASE-NOTES-3.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.34.md|RELEASE-NOTES-2.34]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.51.md|RELEASE-NOTES-2.51]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.41.md|RELEASE-NOTES-2.41]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.10.md|RELEASE-NOTES-2.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.9.md|RELEASE-NOTES-3.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.24.md|RELEASE-NOTES-2.24]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.55.md|RELEASE-NOTES-2.55]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.30.md|RELEASE-NOTES-2.30]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.20.md|RELEASE-NOTES-2.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.14.md|RELEASE-NOTES-2.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.45.md|RELEASE-NOTES-2.45]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.54.md|RELEASE-NOTES-2.54]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.31.md|RELEASE-NOTES-2.31]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.21.md|RELEASE-NOTES-2.21]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.15.md|RELEASE-NOTES-2.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.44.md|RELEASE-NOTES-2.44]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.35.md|RELEASE-NOTES-2.35]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.9.md|RELEASE-NOTES-2.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.50.md|RELEASE-NOTES-2.50]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.40.md|RELEASE-NOTES-2.40]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.11.md|RELEASE-NOTES-2.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.8.md|RELEASE-NOTES-3.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.25.md|RELEASE-NOTES-2.25]]