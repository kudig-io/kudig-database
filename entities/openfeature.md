---
title: OpenFeature [entities]
description: '## 概述'
summary: 'OpenFeature 是特性标志（Feature Flag）的开放标准，提供供应商无关的统一 API 和多语言 SDK。它允许开发者在不更换代码的情况下切换不同的特性标志提供商，实现渐进式发布、A/B 测试和功能开关。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- openfeature
- crd
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFeature 是什么
- 如何 OpenFeature
trigger_keywords:
- OpenFeature
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenFeature

> **CNCF 状态**: Incubating | **类别**: Supply Chain | **主要语言**: TypeScript, Go, Java, Python

## 概述

OpenFeature 是特性标志（Feature Flag）的开放标准，提供供应商无关的统一 API 和多语言 SDK。它允许开发者在不更换代码的情况下切换不同的特性标志提供商，实现渐进式发布、A/B 测试和功能开关。

## 核心能力

- **供应商无关**: 统一 API 支持多种后端提供商
- **多语言 SDK**: JavaScript, Go, Java, Python, .NET, PHP 等
- **Hooks 机制**: 在标志评估前后执行自定义逻辑
- **上下文支持**: 基于用户、环境等上下文进行评估
- **类型安全**: 支持布尔、字符串、数字、对象类型标志
- **可观测性**: 与追踪、日志系统集成

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **命名规范**: 使用清晰的标志名称如 `feature-name-enabled`
- **默认值**: 总是提供合理的默认值
- **上下文丰富**: 传递足够的上下文信息支持精准定向
- **清理旧标志**: 定期删除不再使用的特性标志
- **监控集成**: 使用 Hooks 与可观测性系统集成

## 架构定位

在 CNCF 生态中，openfeature 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kagent]] — Kagent
- [[devspace]] — DevSpace
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfeature
- [[entities/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
