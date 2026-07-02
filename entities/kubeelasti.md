---
title: KubeElastic (entities)
description: '## 概述'
summary: 'KubeElastic 是一个 Kubernetes 原生的弹性伸缩和资源优化平台，专注于基于实时负载和成本的智能资源调整。它结合机器学习预测算法，自动调整 Pod 资源配额（VPA）和副本数（HPA），同时优化集群节点利用率，帮助用户在保证性能 SLO 的前提下降低云成本。'
category: entities
tags:
- k8s
- cncf
- observability
- kubeelasti
- prometheus
- grafana
- hpa
- vpa
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeElastic 是什么
- 如何 KubeElastic
trigger_keywords:
- KubeElastic
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---



# KubeElastic

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

KubeElastic 是一个 Kubernetes 原生的弹性伸缩和资源优化平台，专注于基于实时负载和成本的智能资源调整。它结合机器学习预测算法，自动调整 Pod 资源配额（VPA）和副本数（HPA），同时优化集群节点利用率，帮助用户在保证性能 SLO 的前提下降低云成本。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进启用**: 先以 Dry-run 模式观察推荐值，确认合理后再启用自动调整
- **SLO 优先**: 配置合理的性能 SLO，避免激进缩容影响服务质量
- **预测校准**: 定期检查预测准确性，调整模型参数
- **Spot 容错**: 对使用 Spot 实例的工作负载配置 checkpoint 和重试策略
- **监控告警**: 配置成本和资源利用率告警，跟踪优化效果

## 架构定位

在 CNCF 生态中，[[entities/kubeelasti.md|kubeelasti]] 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeelasti
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
