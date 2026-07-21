---
title: Identity & Access Management
description: 身份与访问知识域 — RBAC 矩阵、OIDC 集成、Vault 密钥管理、ServiceAccount Token、Pod Security Admission
category: subdomain
tags:
- rbac
- oidc
- vault
- secrets-management
- service-account
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 身份与访问 Identity & Access

> Kubernetes 认证、授权、密钥管理的全链路安全实践。

## 认证授权架构

| 层次 | 机制 | 工具 |
|------|------|------|
| 认证 (AuthN) | OIDC/X.509/Token | Dex/Keycloak/LDAP |
| 授权 (AuthZ) | RBAC/ABAC | K8s RBAC/OPA |
| 密钥管理 | Secrets/External | Vault/ESO/Sealed Secrets |
| Pod 安全 | PSA/SCC | Pod Security Admission |
| 服务身份 | SPIFFE/mTLS | SPIRE/cert-manager |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[安全/身份与访问/01-authentication-authorization-system.md\|认证授权体系]] | K8s AuthN/AuthZ 架构 | intermediate |
| [[安全/身份与访问/02-pod-security-admission-deep-dive.md\|PSA 深度解析]] | Pod Security Admission 实践 | advanced |
| [[安全/身份与访问/03-service-account-token-management.md\|SA Token 管理]] | Token 生命周期与安全 | intermediate |
| [[安全/身份与访问/04-oidc-identity-provider-integration.md\|OIDC 集成]] | 企业 IdP 对接 | advanced |
| [[安全/身份与访问/05-vault-enterprise-secrets-management.md\|Vault 密钥管理]] | HashiCorp Vault 企业实践 | advanced |
| [[安全/身份与访问/07-rbac-matrix-configuration.md\|RBAC 矩阵]] | 最小权限原则实践 | intermediate |
| [[安全/身份与访问/11-secret-management-tools.md\|密钥管理工具]] | Vault/ESO/Sealed 对比 | intermediate |
| [[安全/身份与访问/99-vault-k8s-secrets-guide.md\|Vault K8s 指南]] | Vault + K8s 完整指南 | advanced |

## 密钥管理最佳实践

- 禁止在 YAML/代码中硬编码密钥
- 使用 External Secrets Operator 自动同步
- 启用 etcd 静态加密（EncryptionConfiguration）
- ServiceAccount Token 设置有限过期时间
- 定期轮换证书与密钥（cert-manager 自动化）

## Related

- [[安全/策略治理/index.md|策略治理]]
- [[安全/合规审计/index.md|合规审计]]
- [[安全/零信任架构/index.md|零信任架构]]
