---
title: OpenTelemetry
description: '## 概述'
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

# OpenTelemetry

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go, Java, Python, JS 等

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，opentelemetry 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/observability-pillars.md|observability-pillars]]

## Related

- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[06-containerd-observability]] — containerd 可观测性
- [[stacker]] — Stacker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/23-kubernetes-opentelemetry-native-observability.md|23-kubernetes-opentelemetry-native-observability]]
- [[domain-06-observability/03-opentelemetry-distributed-tracing.md|03-opentelemetry-distributed-tracing]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting.md|02-opentelemetry-troubleshooting]]
- [[domain-19-landscape-references/incubating/opentelemetry/opentelemetry.md|opentelemetry]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.43.md|RELEASE-NOTES-0.43]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.129.md|RELEASE-NOTES-0.129]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.26.md|RELEASE-NOTES-0.26]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.77.md|RELEASE-NOTES-0.77]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.84.md|RELEASE-NOTES-0.84]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.94.md|RELEASE-NOTES-0.94]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.67.md|RELEASE-NOTES-0.67]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.36.md|RELEASE-NOTES-0.36]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.139.md|RELEASE-NOTES-0.139]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.53.md|RELEASE-NOTES-0.53]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.22.md|RELEASE-NOTES-0.22]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.80.md|RELEASE-NOTES-0.80]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.119.md|RELEASE-NOTES-0.119]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.47.md|RELEASE-NOTES-0.47]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.148.md|RELEASE-NOTES-0.148]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.57.md|RELEASE-NOTES-0.57]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.109.md|RELEASE-NOTES-0.109]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.90.md|RELEASE-NOTES-0.90]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.32.md|RELEASE-NOTES-0.32]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.63.md|RELEASE-NOTES-0.63]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.23.md|RELEASE-NOTES-0.23]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.81.md|RELEASE-NOTES-0.81]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.118.md|RELEASE-NOTES-0.118]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.149.md|RELEASE-NOTES-0.149]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.46.md|RELEASE-NOTES-0.46]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.56.md|RELEASE-NOTES-0.56]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.108.md|RELEASE-NOTES-0.108]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.91.md|RELEASE-NOTES-0.91]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.33.md|RELEASE-NOTES-0.33]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.62.md|RELEASE-NOTES-0.62]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.42.md|RELEASE-NOTES-0.42]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.27.md|RELEASE-NOTES-0.27]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.128.md|RELEASE-NOTES-0.128]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.76.md|RELEASE-NOTES-0.76]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.85.md|RELEASE-NOTES-0.85]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.95.md|RELEASE-NOTES-0.95]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.66.md|RELEASE-NOTES-0.66]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.138.md|RELEASE-NOTES-0.138]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.37.md|RELEASE-NOTES-0.37]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.52.md|RELEASE-NOTES-0.52]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.146.md|RELEASE-NOTES-0.146]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.49.md|RELEASE-NOTES-0.49]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.117.md|RELEASE-NOTES-0.117]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.123.md|RELEASE-NOTES-0.123]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.133.md|RELEASE-NOTES-0.133]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.107.md|RELEASE-NOTES-0.107]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.59.md|RELEASE-NOTES-0.59]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.79.md|RELEASE-NOTES-0.79]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.28.md|RELEASE-NOTES-0.28]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.127.md|RELEASE-NOTES-0.127]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.113.md|RELEASE-NOTES-0.113]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.142.md|RELEASE-NOTES-0.142]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.103.md|RELEASE-NOTES-0.103]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.137.md|RELEASE-NOTES-0.137]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.38.md|RELEASE-NOTES-0.38]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.69.md|RELEASE-NOTES-0.69]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.78.md|RELEASE-NOTES-0.78]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.126.md|RELEASE-NOTES-0.126]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.29.md|RELEASE-NOTES-0.29]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.112.md|RELEASE-NOTES-0.112]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.143.md|RELEASE-NOTES-0.143]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.102.md|RELEASE-NOTES-0.102]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.39.md|RELEASE-NOTES-0.39]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.136.md|RELEASE-NOTES-0.136]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.68.md|RELEASE-NOTES-0.68]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.48.md|RELEASE-NOTES-0.48]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.147.md|RELEASE-NOTES-0.147]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.116.md|RELEASE-NOTES-0.116]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.122.md|RELEASE-NOTES-0.122]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.132.md|RELEASE-NOTES-0.132]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.106.md|RELEASE-NOTES-0.106]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.58.md|RELEASE-NOTES-0.58]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.125.md|RELEASE-NOTES-0.125]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.88.md|RELEASE-NOTES-0.88]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.140.md|RELEASE-NOTES-0.140]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.111.md|RELEASE-NOTES-0.111]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.101.md|RELEASE-NOTES-0.101]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.98.md|RELEASE-NOTES-0.98]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.135.md|RELEASE-NOTES-0.135]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.115.md|RELEASE-NOTES-0.115]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.144.md|RELEASE-NOTES-0.144]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.121.md|RELEASE-NOTES-0.121]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.131.md|RELEASE-NOTES-0.131]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.0.md|RELEASE-NOTES-0.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.105.md|RELEASE-NOTES-0.105]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.114.md|RELEASE-NOTES-0.114]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.145.md|RELEASE-NOTES-0.145]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.120.md|RELEASE-NOTES-0.120]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.130.md|RELEASE-NOTES-0.130]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.104.md|RELEASE-NOTES-0.104]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.124.md|RELEASE-NOTES-0.124]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.89.md|RELEASE-NOTES-0.89]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.141.md|RELEASE-NOTES-0.141]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.110.md|RELEASE-NOTES-0.110]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.100.md|RELEASE-NOTES-0.100]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.99.md|RELEASE-NOTES-0.99]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.134.md|RELEASE-NOTES-0.134]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.82.md|RELEASE-NOTES-0.82]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.45.md|RELEASE-NOTES-0.45]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.55.md|RELEASE-NOTES-0.55]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.61.md|RELEASE-NOTES-0.61]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.30.md|RELEASE-NOTES-0.30]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.92.md|RELEASE-NOTES-0.92]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.41.md|RELEASE-NOTES-0.41]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.86.md|RELEASE-NOTES-0.86]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.75.md|RELEASE-NOTES-0.75]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.24.md|RELEASE-NOTES-0.24]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.34.md|RELEASE-NOTES-0.34]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.65.md|RELEASE-NOTES-0.65]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.96.md|RELEASE-NOTES-0.96]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.51.md|RELEASE-NOTES-0.51]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.40.md|RELEASE-NOTES-0.40]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.87.md|RELEASE-NOTES-0.87]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.74.md|RELEASE-NOTES-0.74]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.25.md|RELEASE-NOTES-0.25]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.35.md|RELEASE-NOTES-0.35]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.64.md|RELEASE-NOTES-0.64]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.97.md|RELEASE-NOTES-0.97]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.50.md|RELEASE-NOTES-0.50]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.83.md|RELEASE-NOTES-0.83]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.21.md|RELEASE-NOTES-0.21]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.70.md|RELEASE-NOTES-0.70]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.44.md|RELEASE-NOTES-0.44]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.54.md|RELEASE-NOTES-0.54]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.60.md|RELEASE-NOTES-0.60]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.31.md|RELEASE-NOTES-0.31]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.93.md|RELEASE-NOTES-0.93]]
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
