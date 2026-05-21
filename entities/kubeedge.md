---
title: KubeEdge
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- edge
- kubeedge
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
- KubeEdge 是什么
- 如何 KubeEdge
trigger_keywords:
- KubeEdge
prerequisites:
- kubectl-basics
---

# KubeEdge

> **CNCF 状态**: Graduated | **类别**: Edge | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- matchExpressions:
- key: "node-name"
- edge-node-1
- propertyName: temperature
- name: temperature
- name: app

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，kubeedge 属于 **Edge** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[bank-vaults]] — Bank-Vaults
- [[thanos]] — Thanos
- [[03-containerd-security-hardening]] — containerd 安全加固
- [[k0s]] — K0s
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/16-kubernetes-edge-computing-kubeedge-practice.md|16-kubernetes-edge-computing-kubeedge-practice]]
- [[domain-15-specialized-tech/03-kubeedge-architecture-deployment.md|03-kubeedge-architecture-deployment]]
- [[domain-15-specialized-tech/04-kubeedge-device-edge-apps.md|04-kubeedge-device-edge-apps]]
- [[domain-01-cluster-fundamentals/09-edge-computing-kubeedge.md|09-edge-computing-kubeedge]]
- [[domain-19-landscape-references/graduated/kubeedge/kubeedge.md|kubeedge]]
- [[entities/interlink.md|InterLink]]
- [[entities/kairos.md|Kairos]]
- [[references/k8s-cloud-provider-comparison|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
