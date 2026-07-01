---
title: Hexa
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- hexa
- istio
- opa
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hexa 是什么
- 如何 Hexa
trigger_keywords:
- Hexa
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
created: "2026-05-23"
---

# Hexa

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Hexa 是一个统一的策略编排引擎，使用 IDQL (Identity Query Language) 作为通用策略语言，实现跨多个云平台和授权系统的访问控制策略管理。它支持将策略从一个授权系统（如 AWS IAM、Azure RBAC、Google IAP）翻译和同步到另一个系统，避免了在不同平台上重复维护相似策略的问题。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **统一策略定义**: 使用 IDQL 作为策略的唯一真相源，避免在各平台分别维护
- **版本控制**: 将 IDQL 策略文件纳入 Git 管理，启用策略变更审计
- **渐进式迁移**: 先发现已有策略，验证翻译正确性后再同步
- **最小权限**: IDQL 策略设计遵循最小权限原则，默认拒绝
- **条件表达式**: 善用 condition 字段实现 IP 限制、时间窗口等动态策略

## 架构定位

在 CNCF 生态中，hexa 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[03-istio-security-hardening]] — Istio 安全加固
- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[paralus]] — Paralus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hexa
- [[entities/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
