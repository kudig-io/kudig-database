---
title: containerd 可观测性
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- 06-containerd-observability
- etcd
- prometheus
- grafana
- containerd
- falco
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 可观测性 是什么
- 如何 containerd 可观测性
trigger_keywords:
- containerd
- 可观测性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd 可观测性

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

title: containerd 分布式追踪与可观测性

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，06-containerd-observability 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[falco]]
- [[deployment]]

## Related

- [[spiderpool]] — Spiderpool
- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 06-containerd-observability
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
