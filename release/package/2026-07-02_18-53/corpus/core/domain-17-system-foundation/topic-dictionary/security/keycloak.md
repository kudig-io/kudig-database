---
title: Keycloak 身份管理
description: Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是
  Kube...
summary: Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是
  Kube...
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- sso
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Keycloak 身份管理 是什么
- Keycloak 详解
trigger_keywords:
- Keycloak 身份管理
- Keycloak
- dictionary
prerequisites:
- kubernetes
---



# Keycloak 身份管理（Keycloak）

## 概述

Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是 Kubernetes 生态中最常用的外部身份提供者之一。

## 核心概念/原理

- **SSO 平台**：统一的单点登录和身份管理
- **多协议**：支持 OIDC、SAML 2.0、LDAP、Kerberos
- **用户管理**：完整的用户/组/角色管理和自助服务
- **Red Hat 支持**：Red Hat SSO 的开源上游

## 关键机制或特性

- Realm（域）隔离的多租户管理
- Identity Broker（联邦身份代理）连接外部 IdP
- 社交登录（Google/GitHub/Facebook 等）
- 用户自助服务（注册/密码重置/账户管理）
- Fine-Grained Admin Permissions
- OTP/MFA 多因素认证
- 与 Dex 互补（Keycloak 作为 Dex 后端）

## 使用场景与最佳实践

- 企业级 SSO 和身份管理平台
- Kubernetes 集群的外部 OIDC 提供者
- 多应用/多服务的统一认证授权
- 用户自助服务和生命周期管理
- 合规要求下的审计和访问控制

## 参考链接

- https://www.keycloak.org/
- https://github.com/keycloak/keycloak

## Related

- [[domain-17-system-foundation/topic-dictionary/security/dex.md|Dex]]
- [[domain-17-system-foundation/topic-dictionary/security/oauth2-proxy.md|oauth2-proxy]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
