---
title: Open Cluster Management (OCM)
description: '## 概述'
summary: 'Open Cluster Management (OCM) 是一个社区驱动的多集群管理平台，提供 Kubernetes 多集群编排的核心能力。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。'
category: entities
tags:
- k8s
- cncf
- orchestration
- open-cluster-management
- prometheus
- grafana
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Cluster Management (OCM) 是什么
- 如何 Open Cluster Management (OCM)
trigger_keywords:
- Open
- Cluster
- Management
- OCM
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[entities/open-cluster-management.md|Open Cluster Management]] (OCM)

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Open Cluster Management (OCM) 是一个社区驱动的多集群管理平台，提供 Kubernetes 多集群编排的核心能力。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **集群组织**: 使用 ManagedClusterSet 按环境、区域或团队组织集群
- **渐进式部署**: 先使用 `inform` 模式验证策略影响，再切换为 `enforce`
- **Placement 策略**: 利用 Spread 和 Steady 策略平衡负载和稳定性
- **状态反馈**: 在 ManifestWork 中配置 feedbackRules 获取资源状态
- **Addon 管理**: 使用 Addon 框架扩展能力，避免直接在 Spoke 集群操作
- **安全模型**: 遵循最小权限原则，Klusterlet 仅需访问其对应 namespace

## 架构定位

在 CNCF 生态中，open-cluster-management 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[fluid]] — Fluid
- storage.md|cncf-storage]] — CNCF 存储与数据库项目全景
- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- open-cluster-management
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
