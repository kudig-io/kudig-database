---
title: KubeVirt
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- kubevirt
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
- KubeVirt 是什么
- 如何 KubeVirt
trigger_keywords:
- KubeVirt
prerequisites:
- kubectl-basics
---

# KubeVirt

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，kubevirt 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[carvel]] — Carvel
- [[holmesgpt]] — HolmesGPT
- [[ko]] — ko
- [[openfunction]] — OpenFunction
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/incubating/kubevirt/kubevirt.md|kubevirt]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
