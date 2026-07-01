---
title: kgateway
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- kgateway
- istio
- envoy
- gateway
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kgateway 是什么
- 如何 kgateway
trigger_keywords:
- kgateway
prerequisites:
- kubectl-basics
- service-mesh-basics
created: "2026-05-23"
---

# kgateway

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

K Gateway（原 Gloo Gateway）是一个基于 Envoy 的 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] Gateway，完全实现了 Kubernetes Gateway API 标准。它为 Kubernetes 集群提供南北向流量管理、API 路由、认证授权、限流、请求转换等能力，同时支持将流量路由到 Kubernetes [[Service|Service]]、外部服务、Lambda 函数等多种上游目标。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Gateway API 优先**: 使用标准 Gateway API 资源定义路由，保持可移植性
- **TLS 终止**: 在 Gateway 层终止 TLS，后端使用明文通信减少复杂度
- **限流分层**: 组合全局限流和本地限流实现多层保护
- **健康检查**: 配置上游健康检查，自动剔除问题后端
- **灰度发布**: 利用 HTTPRoute 的 weight 字段实现金丝雀发布

## 架构定位

在 CNCF 生态中，kgateway 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]

## Related

- [[kuberhealthy]] — Kuberhealthy
- [[tokenetes]] — Tokenetes
- [[dex]] — Dex
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kgateway
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
