---
title: OpenCost [entities]
description: '## 概述'
summary: 'OpenCost 是 Kubernetes 成本监控的开源标准。它提供实时成本分配、多维度成本分析和优化建议，帮助团队了解和优化 Kubernetes 基础设施支出。'
category: entities
tags:
- k8s
- cncf
- cost
- opencost
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
- OpenCost 是什么
- 如何 OpenCost
trigger_keywords:
- OpenCost
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenCost

> **CNCF 状态**: Incubating | **类别**: Cost | **主要语言**: Go

## 概述

OpenCost 是 Kubernetes 成本监控的开源标准。它提供实时成本分配、多维度成本分析和优化建议，帮助团队了解和优化 Kubernetes 基础设施支出。

## 核心能力

- **实时成本监控**: 分钟级别的成本数据采集
- **多维度分析**: 按命名空间、标签、服务、团队分析成本
- **多云支持**: AWS、Azure、GCP、私有云定价集成
- **Prometheus 集成**: 以 Prometheus 指标格式暴露数据
- **闲置资源检测**: 识别未使用的资源
- **成本分配**: 支持共享资源成本分摊

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **标签策略**: 使用一致的标签（team, app, env）便于成本分配
- **定期审查**: 每周检查成本报告，识别异常增长
- **预算告警**: 设置成本阈值告警
- **资源配额**: 结合成本数据设置命名空间配额
- **闲置资源**: 定期清理未使用的 PV 和负载均衡器

## 架构定位

在 CNCF 生态中，opencost 属于 **Cost** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/observability-pillars.md|observability-pillars]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[piraeus-datastore]] — Piraeus Datastore
- [[k8up]] — K8up
- [[parsec]] — Parsec
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opencost
- observability|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
