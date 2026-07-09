---
title: KubeStellar [entities]
description: '## 概述'
summary: 'KubeStellar 是一个多集群配置管理和工作负载分发平台，专注于将 Kubernetes 资源从中心控制面高效地分发到大量边缘集群。它采用 kcp（Kubernetes-like Control Plane）作为核心，支持管理数千个集群，特别适合边缘计算、零售、IoT 等需要管理大量分布式集群的场景。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubestellar
- containerd
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
- KubeStellar 是什么
- 如何 KubeStellar
trigger_keywords:
- KubeStellar
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeStellar

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeStellar 是一个多集群配置管理和工作负载分发平台，专注于将 Kubernetes 资源从中心控制面高效地分发到大量边缘集群。它采用 kcp（Kubernetes-like Control Plane）作为核心，支持管理数千个集群，特别适合边缘计算、零售、IoT 等需要管理大量分布式集群的场景。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **集群标签**: 建立统一的边缘集群标签体系（location, type, tier）
- **渐进分发**: 使用 BindingPolicy 的集群选择器逐步扩大分发范围
- **状态监控**: 配置状态汇总，在控制面统一监控所有边缘集群状态
- **断网设计**: 边缘应用设计为可在断网情况下独立运行
- **版本控制**: 使用 Workspace 隔离不同版本的工作负载配置

## 架构定位

在 CNCF 生态中，kubestellar 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[05-containerd-windows-support]] — [[containerd|containerd]]rd Windows 支持|containerd Windows 支持]]
- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kcp]] — kcp
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubestellar
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
