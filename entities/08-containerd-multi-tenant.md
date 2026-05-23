---
title: containerd 多租户
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- 08-containerd-multi-tenant
- prometheus
- grafana
- containerd
- networkpolicy
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 多租户 是什么
- 如何 containerd 多租户
trigger_keywords:
- containerd
- 多租户
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# containerd 多租户

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

title: containerd 多租户与共享集群配置

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，08-containerd-multi-tenant 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|[[NetworkPolicy|networkpolicy]]]]
- [[deployment]]

## Related

- [[k0s]] — K0s
- [[kubeedge]] — KubeEdge
- [[telepresence]] — Telepresence
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 08-containerd-multi-tenant