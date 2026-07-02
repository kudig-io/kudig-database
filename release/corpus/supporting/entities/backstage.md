---
title: Backstage [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- platform
- backstage
- prometheus
- grafana
- argocd
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
- Backstage 是什么
- 如何 Backstage
trigger_keywords:
- Backstage
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---



# Backstage

> **CNCF 状态**: Incubating | **类别**: Platform | **主要语言**: TypeScript

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，backstage 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[wasmedge]] — WasmEdge
- [[entities/cncf-runtime.md|cncf-runtime]] — CNCF 容器运行时与工具链项目全景
- [[04-containerd-upgrade-migration]] — [[containerd|containerd]]rd 升级迁移|containerd 升级迁移]]
- [[spin]] — Spin
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-backstage-deployment
- 04-backstage-catalog-techdocs
- 99-backstage-idp-guide
- 05-backstage-scaffolder-templates
- backstage
- [[concepts/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[concepts/platform-engineering-idp.md|Platform Engineering and Internal Developer Platforms]] — Cross-reference
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
