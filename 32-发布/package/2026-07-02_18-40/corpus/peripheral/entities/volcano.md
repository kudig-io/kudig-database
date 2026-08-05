---
title: Volcano [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- volcano
- scheduler
- containerd
- harbor
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
- Volcano 是什么
- 如何 Volcano
trigger_keywords:
- Volcano
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Volcano

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，volcano 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]
- [[pod-lifecycle]]
- [[entities/kube-scheduler.md|kube-scheduler]]

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/core/entities/09-containerd-multi-tenant]] — [[containerd|containerd]]rd 多租户|containerd 多租户]]租户|多租户]]
- [[harbor]] — Harbor
- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- volcano
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
