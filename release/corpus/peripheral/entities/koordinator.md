---
title: Koordinator (entities)
description: '## 概述'
summary: 'Koordinator 是一个基于 QoS 的 Kubernetes 混合调度系统，专为提高集群资源利用率而设计。它通过精细化的资源管理和混部（co-location）技术，在保证延迟敏感型（LS）工作负载 SLO 的同时，充分利用空闲资源运行尽力而为型（BE）任务，实现 60%+ 的集群利用率。'
category: entities
tags:
- k8s
- cncf
- orchestration
- koordinator
- scheduler
- crd
- operator
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Koordinator 是什么
- 如何 Koordinator
trigger_keywords:
- Koordinator
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---



# Koordinator

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Koordinator 是一个基于 QoS 的 Kubernetes 混合调度系统，专为提高集群资源利用率而设计。它通过精细化的资源管理和混部（co-location）技术，在保证延迟敏感型（LS）工作负载 SLO 的同时，充分利用空闲资源运行尽力而为型（BE）任务，实现 60%+ 的集群利用率。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进混部**: 从低资源利用率的集群开始，逐步提高 BE 工作负载比例
- **QoS 分级**: 严格按业务重要性配置 QoS 级别，确保核心服务 SLO
- **CPU Burst**: 为突发流量的在线服务启用 CPU Burst，减少延迟抖动
- **资源画像**: 利用 Koordlet 收集的实际资源使用数据优化资源 request
- **GPU 共享**: 推理服务使用 GPU 共享调度，提升 GPU 利用率
- **弹性 Quota**: 跨团队使用弹性 Quota 允许资源借用，提高整体效率

## 架构定位

在 CNCF 生态中，koordinator 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[entities/kube-scheduler.md|kube-scheduler]]

## Related

- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[devfile]] — Devfile
- [[cohdi]] — Cohdi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- koordinator
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
