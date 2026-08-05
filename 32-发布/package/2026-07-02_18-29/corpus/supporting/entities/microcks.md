---
title: Microcks (entities)
description: '## 概述'
summary: 'Microcks 是一个 API Mock 和测试平台，用于将 OpenAPI、AsyncAPI、gRPC、GraphQL 和 SOAP 的契约规范自动转换为 Mock 服务和集成测试。它帮助开发团队在微服务开发中实现 API 优先（API-First）的工作流，加速并行开发和契约测试。'
category: entities
tags:
- k8s
- cncf
- orchestration
- microcks
- containerd
- rook
- kafka
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
- Microcks 是什么
- 如何 Microcks
trigger_keywords:
- Microcks
prerequisites:
- kubectl-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Microcks

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Java, TypeScript

## 概述

Microcks 是一个 API Mock 和测试平台，用于将 OpenAPI、AsyncAPI、gRPC、GraphQL 和 SOAP 的契约规范自动转换为 Mock 服务和集成测试。它帮助开发团队在微服务开发中实现 API 优先（API-First）的工作流，加速并行开发和契约测试。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **API-First**: 先定义 API 规范，Microcks 生成 Mock，前后端并行开发
- **契约测试**: 在 CI 中运行契约测试，确保服务实现符合 API 规范
- **异步 API**: 使用 AsyncAPI 规范 Mock Kafka/MQTT 消息
- **环境配置**: 为开发、测试环境部署独立的 Microcks 实例
- **版本管理**: API 规范版本化管理，Mock 跟随版本自动更新

## 架构定位

在 CNCF 生态中，microcks 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[32-发布/package/2026-07-02_18-29/corpus/core/entities/01-containerd-v2-features]] — [[containerd|containerd]]rd 2.0 新特性|containerd 2.0 新特性]]
- [[karmada]] — Karmada
- [[rook]] — Rook
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- microcks
- [[entities/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
