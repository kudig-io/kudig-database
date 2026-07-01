---
title: OpenFGA (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- openfga
- envoy
- rbac
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
- OpenFGA 是什么
- 如何 OpenFGA
trigger_keywords:
- OpenFGA
prerequisites:
- kubectl-basics
- tls-basics
---



# OpenFGA

> **CNCF 状态**: Incubating | **类别**: Security | **主要语言**: Go

## 概述

OpenFGA 是细粒度授权（Fine-Grained Authorization）系统，基于 Google Zanzibar 论文设计。它提供灵活的关系型访问控制，支持复杂的权限模型如 RBAC、ABAC 和 ReBAC。

## 核心能力

- **关系型授权**: 基于用户、对象、关系的灵活模型
- **高性能**: 毫秒级权限检查响应
- **DSL 建模**: 简洁的授权模型定义语言
- **多租户**: 原生支持多个授权模型隔离
- **SDK 支持**: Go、Node.js、Python、Java、.NET SDK
- **可扩展**: 水平扩展支持海量权限数据

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模型设计**: 先设计清晰的权限模型，避免过度复杂
- **批量操作**: 使用批量写入和检查减少网络往返
- **缓存策略**: 在应用层缓存高频权限检查结果
- **审计日志**: 记录所有权限变更用于合规审计
- **测试覆盖**: 使用 OpenFGA 测试工具验证授权逻辑

## 架构定位

在 CNCF 生态中，openfga 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[dapr]] — Dapr
- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[zot]] — zot
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfga
- [[entities/kubearmor.md|[[KubeArmor|KubeArmor]]]]
- [[entities/tokenetes.md|Tokenetes]]
- [[entities/containerssh.md|ContainerSSH]]
- [[entities/parsec.md|Parsec]]
- [[entities/athenz.md|Athenz]]
- [[entities/keylime.md|Keylime]]
- [[entities/cartography.md|Cartography]]
- [[entities/bank-vaults.md|Bank-Vaults]]
- [[entities/hexa.md|Hexa]]
- [[entities/paralus.md|Paralus]]
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
