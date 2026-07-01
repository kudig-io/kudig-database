---
title: Kuadrant (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- kuadrant
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
- Kuadrant 是什么
- 如何 Kuadrant
trigger_keywords:
- Kuadrant
prerequisites:
- kubectl-basics
- tls-basics
created: "2026-05-23"
---

# Kuadrant

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, Rust

## 概述

Kuadrant 是一个 Kubernetes Gateway API 的策略引擎，为 Gateway API 添加 API 管理能力，包括认证、授权、限流和 DNS 管理。它通过 Policy Attachment 模式将策略附加到 Gateway API 资源（Gateway、HTTPRoute）上，无需修改路由配置即可添加安全和流量管理策略，实现了 Gateway API 原生的 AP...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **策略层级**: Gateway 级设置默认策略，HTTPRoute 级覆盖特定路由的策略
- **限流维度**: 组合用户、IP、路径等多维度实现精细化限流
- **认证分层**: 公开 API 用 API Key，内部 API 用 JWT/mTLS
- **DNS 地理路由**: 多区域部署时配合 DNSPolicy 实现就近访问
- **TLS 自动化**: 配合 cert-manager 实现证书的自动签发和续期

## 架构定位

在 CNCF 生态中，kuadrant 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[open-cluster-management]] — [[entities/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[cdk8s]] — cdk8s (Cloud Development Kit for Kubernetes)
- [[cloud-custodian]] — Cloud Custodian
- [[cert-manager]] — cert-manager
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kuadrant
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
