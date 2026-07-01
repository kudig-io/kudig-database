---
title: Jaeger (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- jaeger
- prometheus
- grafana
- kafka
- elasticsearch
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Jaeger 是什么
- 如何 Jaeger
trigger_keywords:
- Jaeger
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- tracing-basics
created: "2026-05-23"
---

# Jaeger

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **分布式追踪**: 跨服务请求追踪
- **根因分析**: 快速定位问题
- **服务依赖**: 可视化服务关系图
- **性能分析**: 延迟分析和优化
- **多存储后端**: Cassandra、Elasticsearch、Kafka

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，jaeger 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/observability-pillars.md|observability-pillars]]
- [[concepts/storage-model.md|storage-model]]

## Related
- [[concepts/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合

- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[score]] — Score
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- jaeger
- [[entities/k8s-observability-ecosystem.md|[[可观测性体系：指标、日志、链路追踪与混沌工程|可观测性体系：指标、日志、链路追踪与混沌工程]]]] — Cross-reference
- [[entities/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[entities/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[concepts/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[concepts/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
