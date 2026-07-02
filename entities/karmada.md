---
title: Karmada (entities)
description: '## 概述'
summary: 'Karmada（Kubernetes Armada）是开放的多云多集群 Kubernetes 管理系统。它提供统一的 API 来管理跨多个 Kubernetes 集群的工作负载，支持跨集群调度、故障转移和策略驱动的资源分发。'
category: entities
tags:
- k8s
- cncf
- orchestration
- karmada
- etcd
- apiserver
- kubelet
- containerd
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
- Karmada 是什么
- 如何 Karmada
trigger_keywords:
- Karmada
prerequisites:
- kubectl-basics
- etcd-basics
---



# Karmada

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Karmada（Kubernetes Armada）是开放的多云多集群 Kubernetes 管理系统。它提供统一的 API 来管理跨多个 Kubernetes 集群的工作负载，支持跨集群调度、故障转移和策略驱动的资源分发。

## 核心能力

- **多集群管理**: 统一管理多个 Kubernetes 集群
- **跨集群调度**: 基于策略的工作负载分发
- **故障转移**: 自动检测集群问题并迁移工作负载
- **Kubernetes 原生**: 完全兼容 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]]
- **集群联邦**: 统一的资源视图和管理
- **多云支持**: 支持公有云、私有云、边缘集群

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **集群分组**: 使用标签对集群分组（区域、环境、云厂商）
- **渐进式迁移**: 从非关键工作负载开始逐步迁移
- **故障转移测试**: 定期验证故障转移能力
- **资源配额**: 在控制面配置跨集群资源配额
- **网络规划**: 确保集群间网络连通性

## 架构定位

在 CNCF 生态中，karmada 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[deployment]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[entities/kube-apiserver.md|kube-apiserver]]

## Related

- [[entities/virtual-kubelet.md|kubelet]]]] — Virtual Kubelet
- [[kudo]] — KUDO
- [[02-containerd-v2-features]] — containerd 2.0 新特性
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[armada]] — Armada

- 08-multicloud-federation-karmada
- karmada
- [[concepts/etcd x 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
