---
title: KEDA
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- keda
- prometheus
- grafana
- jaeger
- helm
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KEDA 是什么
- 如何 KEDA
trigger_keywords:
- KEDA
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- tracing-basics
---

# KEDA

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，keda 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[score]] — Score
- [[jaeger]] — Jaeger
- [[helm]] — Helm
- [[cloudevents]] — CloudEvents
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-06-observability/99-keda-event-driven-autoscaling-guide.md|99-keda-event-driven-autoscaling-guide]]
- [[domain-19-landscape-references/graduated/keda/keda.md|keda]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
