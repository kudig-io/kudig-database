---
title: Perses (entities)
description: '## 概述'
summary: 'Perses 是一个云原生的 Dashboard 即代码 (Dashboard-as-Code) 可视化平台，用于创建和管理可观测性仪表板。它旨在成为 Grafana 的开源替代方案之一，提供标准化的 Dashboard 定义规范，支持将仪表板作为代码进行版本控制和 GitOps 管理。'
category: entities
tags:
- k8s
- cncf
- observability
- perses
- prometheus
- grafana
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
- Perses 是什么
- 如何 Perses
trigger_keywords:
- Perses
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---



# Perses

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go, TypeScript

## 概述

Perses 是一个云原生的 Dashboard 即代码 (Dashboard-as-Code) 可视化平台，用于创建和管理可观测性仪表板。它旨在成为 Grafana 的开源替代方案之一，提供标准化的 Dashboard 定义规范，支持将仪表板作为代码进行版本控制和 GitOps 管理。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **代码化管理**: 将 Dashboard 定义存储在 Git 仓库，通过 CI/CD 部署
- **变量化**: 使用变量实现 Dashboard 的环境通用性
- **标准化**: 团队统一使用 Perses 定义规范，确保仪表板一致性
- **CRD 集成**: 在 Kubernetes 中使用 PersesDashboard CRD 实现 GitOps
- **数据源配置**: 集中管理 Prometheus 数据源配置

## 架构定位

在 CNCF 生态中，perses 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[pod-lifecycle]]

## Related

- [[kaito]] — KAITO
- [[youki]] — youki
- [[easegress]] — Easegress
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- perses
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
