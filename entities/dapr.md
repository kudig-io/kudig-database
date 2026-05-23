---
title: Dapr (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- dapr
- istio
- redis
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dapr 是什么
- 如何 Dapr
trigger_keywords:
- Dapr
prerequisites:
- kubectl-basics
- service-mesh-basics
- redis-basics
created: "2026-05-23"
---

# Dapr

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- name: redisHost
- name: redisPassword
- name: brokers
- name: consumerGroup

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，dapr 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/vault.md|[[HashiCorp Vault|vault]]]]
- [[deployment]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[02-istio-advanced-traffic-management]] — [[Istio|Istio]]io 高级流量管理|Istio 高级流量管理]]
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-dapr-enterprise-distributed-runtime
- dapr
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
