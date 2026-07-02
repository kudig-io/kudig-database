---
title: Kubewarden [entities]
description: '## 概述'
summary: 'Kubewarden 是一个 Kubernetes 策略引擎，使用 WebAssembly (Wasm) 运行准入策略。它允许使用任何编译为 Wasm 的编程语言 (Rust、Go、C#、Swift 等) 编写策略，并通过 OCI 镜像仓库分发。Kubewarden 支持动态准入控制和审计模式。'
category: entities
tags:
- k8s
- cncf
- policy
- kubewarden
- argocd
- crd
- operator
- wasm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubewarden 是什么
- 如何 Kubewarden
trigger_keywords:
- Kubewarden
prerequisites:
- kubectl-basics
- gitops-basics
---



# Kubewarden

> **CNCF 状态**: Sandbox | **类别**: Policy | **主要语言**: Rust, Go

## 概述

Kubewarden 是一个 Kubernetes 策略引擎，使用 WebAssembly (Wasm) 运行准入策略。它允许使用任何编译为 Wasm 的编程语言 (Rust、Go、C#、Swift 等) 编写策略，并通过 OCI 镜像仓库分发。Kubewarden 支持动态准入控制和审计模式。

## 核心能力

- **WebAssembly 策略**: 使用 Wasm 编写和运行策略
- **多语言支持**: Rust、Go、C#、Swift、Rego 等
- **OCI 分发**: 策略通过 OCI 仓库分发
- **审计模式**: 不阻止请求，只记录违规
- **策略组**: 组合多个策略为逻辑组
- **上下文感知**: 策略可查询集群状态

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进部署**: 先在 monitor 模式运行策略
- **策略复用**: 优先使用官方策略库
- **PolicyServer 隔离**: 为关键策略使用独立 PolicyServer
- **版本锁定**: 使用精确版本标签引用策略
- **测试**: 使用 kwctl 在部署前充分测试
- **监控**: 监控 PolicyServer 资源使用和延迟

## 架构定位

在 CNCF 生态中，kubewarden 属于 **Policy** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[entities/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[entities/external-secrets.md|secrets]]]] — External Secrets Operator
- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubewarden
- [[entities/capsule.md|Capsule]]
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
