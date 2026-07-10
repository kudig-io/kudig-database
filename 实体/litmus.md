---
title: LitmusChaos
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- litmus
- prometheus
- grafana
- istio
- argocd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LitmusChaos 是什么
- 如何 LitmusChaos
trigger_keywords:
- LitmusChaos
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# LitmusChaos

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

[[Litmus|Litmus]] 是云原生混沌工程平台，提供完整的混沌实验编排和管理能力。它帮助团队在受控环境中测试系统弹性，发现潜在问题点并提高系统可靠性。

## 核心能力

- **丰富的实验库**: ChaosHub 提供 50+ 预置混沌实验
- **Kubernetes 原生**: CRD 方式定义和管理混沌实验
- **GitOps 支持**: 混沌即代码，版本控制管理
- **可观测性集成**: Prometheus 指标和 Grafana 仪表盘
- **细粒度控制**: 支持命名空间、标签、注解级别的定向注入
- **多租户**: 支持多团队协作管理混沌实验

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进式测试**: 从低强度实验开始，逐步增加复杂度
- **稳态假设**: 实验前定义清晰的稳态指标和阈值
- **最小爆炸半径**: 限制实验范围，避免影响生产环境
- **自动化集成**: 将混沌实验纳入 CI/CD 流水线
- **游戏日**: 定期组织全团队参与的混沌工程演练

## 架构定位

在 CNCF 生态中，litmus 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/argocd.md|argocd]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[openkruise]] — OpenKruise
- [[02-istio-advanced-traffic-management]] — Istio 高级流量管理
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- litmus
- [[实体/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[实体/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
