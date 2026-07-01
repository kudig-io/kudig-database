---
title: Paralus (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- paralus
- istio
- opa
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
- Paralus 是什么
- 如何 Paralus
trigger_keywords:
- Paralus
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---



# Paralus

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Paralus 是一个 Kubernetes 零信任访问管理平台，为多集群环境提供统一的身份认证、授权和审计能力。它作为 kubectl 和 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] 之间的安全代理层，实现基于身份的细粒度访问控制和完整的操作审计日志。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **零信任**: 所有集群访问通过 Paralus 代理，不直接暴露 API Server
- **JIT 访问**: 为生产集群使用即时访问，避免长期权限授予
- **审计合规**: 利用审计日志满足合规要求，记录所有 kubectl 操作
- **IdP 集成**: 使用企业 IdP (Okta/Azure AD) 统一身份管理
- **最小权限**: 基于项目和命名空间配置最小权限角色

## 架构定位

在 CNCF 生态中，paralus 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[pod-lifecycle]]

## Related

- [[distribution]] — Distribution
- [[03-istio-security-hardening]] — [[Istio|Istio]]io 安全加固|Istio 安全加固]]
- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- paralus
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
