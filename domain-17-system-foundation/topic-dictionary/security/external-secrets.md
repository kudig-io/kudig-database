---
title: External Secrets Operator
description: 'External Secrets Operator（ESO）是 Kubernetes 原生的密钥同步工具，从外部密钥管理系统（Vault、AWS Secrets...'
category: dictionary
tags:
- k8s
- glossary
- external-secrets
- secrets-management
- security
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- External Secrets Operator 是什么
- External Secrets Operator 详解
trigger_keywords:
- External Secrets Operator
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
created: 2026-05
---

# External Secrets Operator

> **英文名**: External Secrets Operator

## 概述

External Secrets Operator（ESO）是 Kubernetes 原生的密钥同步工具，从外部密钥管理系统（Vault、AWS Secrets Manager、Azure Key Vault 等）自动同步密钥到 K8s Secret 资源。

## 核心概念/原理

### 核心资源

| 资源 | 功能 |
|------|------|
| SecretStore | 命名空间级的外部密钥源配置 |
| ClusterSecretStore | 集群级的外部密钥源配置 |
| ExternalSecret | 声明式的外部密钥同步定义 |
| ClusterExternalSecret | 集群范围的密钥同步 |

### 支持的 Backend

HashiCorp Vault、AWS Secrets Manager、AWS Parameter Store、Azure Key Vault、GCP Secret Manager、1Password、Akeyless 等 20+。

## 关键机制或特性

- **自动同步**：外部密钥变更时自动更新 K8s Secret。
- **Template**：自定义 Secret 的 key 名称和数据格式。
- **Push Secret**：将 K8s Secret 推送到外部存储。
- **Refresh Interval**：配置同步频率。
- 支持假删除（Deletion Policy）保护。

## 使用场景与最佳实践

- 使用 ESO 替代手动管理 K8s Secret。
- 配合 Vault 实现集中式密钥管理。
- 使用 ClusterSecretStore 统一管理所有命名空间的密钥源。
- 为 CI/CD 生成的密钥配置自动同步到 Vault。
- 监控 ESO 的同步状态和错误指标。

## 参考链接

- [External Secrets Operator](https://external-secrets.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/vault.md|Vault]]
- [[domain-17-system-foundation/topic-dictionary/security/secret.md|Secret]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate.md|Certificate]]
- [[domain-17-system-foundation/topic-dictionary/security/service-account.md|Service Account]]
- [[domain-17-system-foundation/topic-dictionary/operations/cert-manager.md|cert-manager]]
