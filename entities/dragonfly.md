---
title: Dragonfly
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- dragonfly
- scheduler
- prometheus
- grafana
- containerd
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dragonfly 是什么
- 如何 Dragonfly
trigger_keywords:
- Dragonfly
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

# Dragonfly

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- regx: ".*\\.example\\.com.*"
- regx: ".*internal.*"

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，dragonfly 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[entities/kube-scheduler.md|kube-scheduler]]

## Related

- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[strimzi]] — Strimzi
- [[hwameistor]] — HwameiStor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/graduated/dragonfly/dragonfly.md|dragonfly]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
