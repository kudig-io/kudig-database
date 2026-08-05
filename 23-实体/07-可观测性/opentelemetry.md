---
title: OpenTelemetry (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- opentelemetry
- prometheus
- grafana
- containerd
- crd
- operator
- wasm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- OpenTelemetry 是什么
- 如何 OpenTelemetry
trigger_keywords:
- OpenTelemetry
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenTelemetry

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go, Java, Python, JS 等

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，opentelemetry 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[22-概念/06-可观测性/observability-pillars.md|observability-pillars]]

## Related

- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[05-containerd-observability]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[stacker]] — Stacker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 23-kubernetes-opentelemetry-native-observability
- 03-opentelemetry-distributed-tracing
- [[19-故障诊断/04-高级排障/structural-12-monitoring-observability/02-opentelemetry-troubleshooting.md|02-opentelemetry-troubleshooting]]
- opentelemetry
- RELEASE-NOTES-0.43
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.129
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.77
- RELEASE-NOTES-0.84
- RELEASE-NOTES-0.94
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.67
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.139
- RELEASE-NOTES-0.53
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.80
- RELEASE-NOTES-0.119
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.47
- RELEASE-NOTES-0.148
- RELEASE-NOTES-0.57
- RELEASE-NOTES-0.109
- RELEASE-NOTES-0.90
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.63
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.81
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.118
- RELEASE-NOTES-0.149
- RELEASE-NOTES-0.46
- RELEASE-NOTES-0.56
- RELEASE-NOTES-0.108
- RELEASE-NOTES-0.91
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.62
- RELEASE-NOTES-0.42
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-0.128
- RELEASE-NOTES-0.76
- RELEASE-NOTES-0.85
- RELEASE-NOTES-0.95
- RELEASE-NOTES-0.66
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.138
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.52
- RELEASE-NOTES-0.146
- RELEASE-NOTES-0.49
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.117
- RELEASE-NOTES-0.123
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.133
- RELEASE-NOTES-0.107
- RELEASE-NOTES-0.59
- RELEASE-NOTES-0.79
- RELEASE-NOTES-0.28
- RELEASE-NOTES-0.127
- RELEASE-NOTES-0.113
- RELEASE-NOTES-0.142
- RELEASE-NOTES-0.103
- RELEASE-NOTES-0.137
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.69
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.78
- RELEASE-NOTES-0.126
- RELEASE-NOTES-0.29
- RELEASE-NOTES-0.112
- RELEASE-NOTES-0.143
- RELEASE-NOTES-0.102
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.136
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.68
- RELEASE-NOTES-0.48
- RELEASE-NOTES-0.147
- RELEASE-NOTES-0.116
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.122
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.132
- RELEASE-NOTES-0.106
- RELEASE-NOTES-0.58
- RELEASE-NOTES-0.125
- RELEASE-NOTES-0.88
- RELEASE-NOTES-0.140
- RELEASE-NOTES-0.111
- RELEASE-NOTES-0.101
- RELEASE-NOTES-0.98
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.135
- RELEASE-NOTES-0.115
- RELEASE-NOTES-0.144
- RELEASE-NOTES-0.121
- RELEASE-NOTES-0.131
- RELEASE-NOTES-0.0
- RELEASE-NOTES-0.105
- RELEASE-NOTES-0.114
- RELEASE-NOTES-0.145
- RELEASE-NOTES-0.120
- RELEASE-NOTES-0.130
- RELEASE-NOTES-0.104
- RELEASE-NOTES-0.124
- RELEASE-NOTES-0.89
- RELEASE-NOTES-0.141
- RELEASE-NOTES-0.110
- RELEASE-NOTES-0.100
- RELEASE-NOTES-0.99
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.134
- RELEASE-NOTES-0.82
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.45
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.55
- RELEASE-NOTES-0.61
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.92
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.86
- RELEASE-NOTES-0.75
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.65
- RELEASE-NOTES-0.96
- RELEASE-NOTES-0.51
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.87
- RELEASE-NOTES-0.74
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.64
- RELEASE-NOTES-0.97
- RELEASE-NOTES-0.50
- RELEASE-NOTES-0.83
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.70
- RELEASE-NOTES-0.44
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.54
- RELEASE-NOTES-0.60
- RELEASE-NOTES-0.31
- RELEASE-NOTES-0.93
- [[23-实体/15-参考与索引/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[22-概念/10-最佳实践/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[22-概念/12-研究/ai-agent-README.md|AI Agent 工程专题]] — Cross-reference
- [[22-概念/12-研究/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
