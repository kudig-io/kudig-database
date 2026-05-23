---
title: Headlamp (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- platform
- headlamp
- prometheus
- grafana
- envoy
- flux
- ingress
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Headlamp 是什么
- 如何 Headlamp
trigger_keywords:
- Headlamp
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- tls-basics
created: "2026-05-23"
---

# Headlamp

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: TypeScript, Go

## 概述

Headlamp 是一个现代化的 Kubernetes Web UI，提供直观的集群管理界面。它可以作为桌面应用、Web 应用或集群内应用运行，支持插件扩展系统，允许用户自定义功能。Headlamp 注重用户体验，提供清晰的资源视图和操作界面。

## 核心能力

- **多平台支持**: 桌面应用 (Electron)、Web 应用、集群内部署
- **多集群管理**: 单一界面管理多个 Kubernetes 集群
- **插件系统**: 通过插件扩展功能
- **实时更新**: 资源状态实时刷新
- **RBAC 集成**: 基于用户权限显示可用操作
- **YAML 编辑**: 内置 YAML 编辑器

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **RBAC 配置**: 为不同用户创建适当权限的 ServiceAccount
- **OIDC 集成**: 生产环境使用 OIDC 而非 Token 认证
- **插件管理**: 只安装必要的插件，定期更新
- **[[Ingress|Ingress]] 安全**: 启用 TLS，配置访问控制
- **资源限制**: 为 Headlamp Pod 设置资源限制
- **审计日志**: 启用 Kubernetes 审计日志跟踪操作

## 架构定位

在 CNCF 生态中，headlamp 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[flux]]
- [[deployment]]
- [[concepts/storage-model|storage-model]]
- [[concepts/secrets-management|secrets-management]]

## Related

- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[zot]] — zot
- [[openfga]] — OpenFGA
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- headlamp
- [[entities/opengitops|OpenGitOps]]
- [[entities/kubeclipper|KubeClipper]]
- [[entities/cozystack|Cozystack]]
- [[entities/kube-rs|kube-rs]]
- [[entities/kagent|Kagent]]
- [[entities/openchoreo|OpenChoreo]]
- [[entities/holmesgpt|HolmesGPT]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
