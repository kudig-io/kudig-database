---
title: kcp
description: '## 概述'
summary: 'kcp 是一个类 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] 服务器，提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kcp
- etcd
- rbac
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kcp 是什么
- 如何 kcp
trigger_keywords:
- kcp
prerequisites:
- kubectl-basics
- etcd-basics
---



# kcp

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

kcp 是一个类 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] 服务器，提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。它利用 Kubernetes 的 API 机制（CRD、控制器、准入控制等），将其从容器编排中解耦出来，作为通用的 API 平台使用。kcp 支持在单个服务器上运行数千个逻辑集群（Workspace），每个 Workspace 拥有独立的 API 视图和资源隔离。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Workspace 层级设计**: 使用组织级 Workspace 管理团队边界，应用级 Workspace 管理服务
- **API 版本管理**: 通过 APIExport 版本化你的平台 API，确保向后兼容
- **Syncer 高可用**: 为每个物理集群部署多副本 Syncer 保证同步可靠性
- **RBAC 策略**: 利用 kcp 的多租户 RBAC 实现最小权限原则
- **监控**: 监控 kcp 的 etcd 存储使用和 API 请求延迟

## 架构定位

在 CNCF 生态中，kcp 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[loxilb]] — LoxiLB
- [[kube-ovn]] — Kube-OVN
- [[flatcar]] — Flatcar Container Linuxux 生产环境速查卡|Linux]]
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kcp
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
