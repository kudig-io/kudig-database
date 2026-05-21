---
title: External Secrets Operator
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- supply-chain
- external-secrets
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- External Secrets Operator 是什么
- 如何 External Secrets Operator
trigger_keywords:
- External
- Secrets
- Operator
prerequisites:
- kubectl-basics
---

# External Secrets Operator

> **CNCF 状态**: Sandbox | **类别**: Supply Chain | **主要语言**: Go

## 概述

External Secrets Operator (ESO) 将外部密钥管理系统（如 AWS Secrets Manager、HashiCorp Vault、Azure Key Vault）的密钥同步到 Kubernetes Secrets，实现安全的密钥管理和自动轮换。

## 核心能力

- **多后端支持**: AWS、Azure、GCP、Vault、1Password 等 20+ 提供商
- **自动同步**: 定期从外部系统同步密钥到 K8s
- **密钥模板**: 支持模板化生成 Secret 内容
- **密钥轮换**: 自动检测和同步密钥更新
- **多租户**: 支持命名空间级别的隔离
- **推送模式**: 支持将 K8s Secret 推送到外部系统

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **最小权限**: 为 ESO 配置最小必需的云平台权限
- **命名空间隔离**: 使用 SecretStore 而非 ClusterSecretStore 实现隔离
- **刷新间隔**: 根据安全需求设置合理的 refreshInterval
- **密钥轮换**: 在外部系统轮换密钥后，ESO 会自动同步
- **监控**: 监控同步状态和错误

## 架构定位

在 CNCF 生态中，external-secrets 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/vault.md|vault]]
- [[operator-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[slimtoolkit]] — SlimToolkit
- [[cni]] — CNI (Container Network Interface)
- [[entities/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[entities/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/external-secrets/external-secrets.md|external-secrets]]
- [[entities/ratify.md|Ratify]]
- [[references/kudig-ecosystem-guide|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
