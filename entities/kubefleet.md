---
title: KubeFleet [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubefleet
- cri-o
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeFleet 是什么
- 如何 KubeFleet
trigger_keywords:
- KubeFleet
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# KubeFleet

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeFleet 是一个多集群资源编排平台，提供跨 Kubernetes 集群的工作负载分发、配置管理和策略驱动的资源放置能力。它通过 Hub-Member 架构和声明式 Placement 策略，实现将 Kubernetes 资源（Deployment、[[Service|Service]]、ConfigMap 等）自动分发到多个成员集群，并支持基于集群属性、资源可用性和自定义策略的智能调度。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Hub 高可用**: Hub 集群使用多副本部署，确保控制面高可用
- **标签规范**: 统一集群标签体系（region、env、tier），便于调度策略编写
- **渐进式发布**: 关键服务使用 RollingUpdate 策略，避免同时更新所有集群
- **资源选择器**: 精确定义 resourceSelectors，避免意外分发不需要的资源
- **监控告警**: 监控 ClusterResourcePlacement 的 status conditions，及时发现分发异常

## 架构定位

在 CNCF 生态中，kubefleet 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[cedar]] — Cedar
- [[cri-o]] — CRI-O
- [[shipwright]] — Shipwright
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubefleet
- [[entities/cncf-orchestration|[[CNCF 编排与应用管理项目全景|CNCF 编排与应用管理项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
