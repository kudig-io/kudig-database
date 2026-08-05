---
title: Kuberhealthy (entities)
description: '## 概述'
summary: 'Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控工具。它通过运行 Kubernetes Job 来执行健康检查，将检查结果以 Prometheus 指标格式输出。支持自定义检查，可以验证 DNS、部署、存储、网络等各方面的集群健康状态。'
category: entities
tags:
- k8s
- cncf
- observability
- kuberhealthy
- prometheus
- grafana
- daemonset
- job
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
- Kuberhealthy 是什么
- 如何 Kuberhealthy
trigger_keywords:
- Kuberhealthy
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kuberhealthy

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控工具。它通过运行 Kubernetes Job 来执行健康检查，将检查结果以 Prometheus 指标格式输出。支持自定义检查，可以验证 DNS、部署、存储、网络等各方面的集群健康状态。

## 核心能力

- **合成监控**: 通过 Kubernetes Job 执行主动健康检查
- **丰富检查项**: DNS、Deployment、[[DaemonSet|DaemonSet]]、Pod 等
- **自定义检查**: 使用任何容器镜像编写自定义检查
- **Prometheus 集成**: 检查结果直接导出为指标
- **CRD 配置**: 使用 KuberhealthyCheck CRD 定义检查
- **多命名空间**: 支持跨命名空间检查

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **检查频率**: 关键检查每 2-5 分钟，非关键 10-15 分钟
- **超时设置**: 合理设置超时避免误报
- **自定义检查**: 针对业务场景编写自定义检查
- **告警集成**: 配置 Prometheus 告警规则

## 架构定位

在 CNCF 生态中，kuberhealthy 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/observability-pillars.md|observability-pillars]]
- [[pod-lifecycle]]

## Related

- [[kubefleet]] — KubeFleet
- [[kuma]] — Kuma
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kuberhealthy
- [[entities/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
