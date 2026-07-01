---
title: Armada (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- armada
- scheduler
- job
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
- Armada 是什么
- 如何 Armada
trigger_keywords:
- Armada
prerequisites:
- kubectl-basics
---



# Armada

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Armada 是一个多集群批处理作业调度系统，专为在多个 Kubernetes 集群上运行大规模批处理工作负载（如 HPC 计算、ML 训练、CI/CD 等）而设计。它提供统一的作业提交入口、跨集群的公平调度、优先级抢占和作业队列管理，能够管理数百万个并发作业在数千个节点上的高效调度。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **队列设计**: 按团队或项目划分队列，设置合理的资源配额和优先级
- **作业分组**: 使用 JobSet 将相关作业分组，便于统一管理和监控
- **资源估算**: 准确设置作业的 resource requests，避免资源浪费或调度失败
- **Executor 分布**: 在不同可用区/区域部署 Executor 集群，提高容灾能力
- **监控 Lookout**: 使用 Lookout UI 监控队列积压和作业完成率

## 架构定位

在 CNCF 生态中，armada 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[pod-lifecycle]]
- [[entities/kube-scheduler.md|kube-scheduler]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[keycloak]] — Keycloak
- [[kubearmor]] — KubeArmor
- [[entities/cncf-cicd.md|cncf-cicd]] — CNCF CI/CD 与发布管理项目全景
- networking.md|cncf-networking]] — CNCF 网络与服务网格项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- digest-2026-05-21-full
- 08-multicloud-federation-karmada
- armada
- karmada
- [[entities/cohdi.md|Cohdi]]
- [[entities/kubefleet.md|KubeFleet]]
- [[entities/clusternet.md|Clusternet]]
- [[entities/kured.md|Kured (KUbernetes REboot Daemon)]]
- [[entities/kubevela.md|KubeVela]]
- [[entities/kubestellar.md|KubeStellar]]
- [[entities/microcks.md|Microcks]]
- [[entities/kudo.md|KUDO]]
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
