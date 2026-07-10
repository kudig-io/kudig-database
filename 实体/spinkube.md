---
title: SpinKube [entities]
description: '## 概述'
summary: 'SpinKube 是一个在 Kubernetes 上运行 WebAssembly (Wasm) 微服务和应用的开源平台。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度、更小的资源占用和更强的安全隔离。'
category: entities
tags:
- k8s
- cncf
- runtime
- spinkube
- prometheus
- containerd
- gateway
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SpinKube 是什么
- 如何 SpinKube
trigger_keywords:
- SpinKube
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SpinKube

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust, Go

## 概述

SpinKube 是一个在 Kubernetes 上运行 WebAssembly (Wasm) 微服务和应用的开源平台。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度、更小的资源占用和更强的安全隔离。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **适用场景**: 高密度短任务、API Gateway、边缘计算、Serverless 函数
- **OCI 分发**: 使用 `spin registry push` 将 Wasm 应用作为 OCI artifact 分发
- **状态管理**: 使用 Spin 的 Key-Value Store 或 SQLite 数据库组件管理状态
- **渐进采用**: 从无状态 API 和辅助微服务开始，逐步扩大 Wasm 工作负载比例
- **监控**: 利用 Kubernetes 标准监控工具监控 SpinApp 的 Pod 状态和资源使用
- **多组件**: 利用 Spin 的多组件模型在一个应用中组合 API 和静态文件服务

## 架构定位

在 CNCF 生态中，spinkube 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[kube-rs]] — kube-rs
- [[02-prometheus-promql-advanced]] — PromQLQL 高级查询|PromQL 高级查询]]
- [[capsule]] — Capsule
- [[spin]] — Spin
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-spinkube-framework
- spinkube
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
