---
title: HashiCorp Vault
description: HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes
  环境中，Vault...
summary: HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes 环境中，Vault...
category: dictionary
tags:
- k8s
- glossary
- vault
- secrets-management
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HashiCorp Vault 是什么
- Vault 详解
trigger_keywords:
- HashiCorp Vault
- Vault
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HashiCorp Vault

> **英文名**: Vault

## 概述

HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes 环境中，Vault 是集中式密钥管理的标准方案。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Secret Engine | 密钥存储和管理（KV、数据库、PKI 等） |
| Auth Method | 身份认证（K8s、LDAP、AppRole 等） |
| Policy | 访问控制策略 |
| Transit | 加密即服务（Encryption as a Service） |
| PKI | 动态证书签发和吊销 |

### K8s 集成方式

- **Vault Agent Sidecar**：自动注入密钥到 Pod。
- **Vault CSI Provider**：通过 CSI 卷挂载密钥。
- **External Secrets Operator**：同步 Vault 密钥到 K8s Secret。

## 关键机制或特性

- **动态凭证**：按需生成短生命周期的数据库凭证、AWS 凭证等。
- **Kubernetes Auth**：使用 ServiceAccount Token 认证 Pod 身份。
- **Auto-Unseal**：使用云 KMS 自动解封 Vault。
- **审计日志**：记录所有密钥访问操作。
- **Secret Rotation**：自动轮转数据库密码和 API 密钥。

## 使用场景与最佳实践

- 生产环境使用 Vault 替代 K8s Secret 管理敏感信息。
- 启用 K8s Auth Method 实现 Pod 级别的密钥访问。
- 使用 Vault Agent Sidecar 自动注入密钥（无需修改应用代码）。
- 配置短期凭证（TTL < 1h）减少密钥泄露风险。
- 启用审计日志满足合规要求。

## 参考链接

- [Vault Official](https://www.vaultproject.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/secret.md|Secret]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate.md|Certificate]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate-authority.md|Certificate Authority]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/security/service-account.md|Service Account]]


<!-- risk-assessed -->
