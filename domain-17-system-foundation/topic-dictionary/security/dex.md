---
title: Dex 身份认证
description: Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitH...
summary: Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitH...
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- oidc
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dex 身份认证 是什么
- Dex 详解
trigger_keywords:
- Dex 身份认证
- Dex
- dictionary
prerequisites:
- kubernetes
---



# Dex 身份认证（Dex）

## 概述

Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitHub 等），为 Kubernetes 和其他应用提供统一的身份认证层。

## 核心概念/原理

- **联邦身份**：充当 IdP 聚合层，统一 LDAP、SAML、GitHub、GitLab、Microsoft 等认证源
- **OIDC 标准**：完整实现 OpenID Connect 协议，兼容所有 OIDC 客户端
- **Kubernetes 原生**：广泛用于 K8s API Server 的 OIDC 认证配置
- **轻量部署**：单二进制，可运行在 K8s 内或独立部署

## 关键机制或特性

- 支持多种 Connector（LDAP、SAML 2.0、GitHub、GitLab、Bitbucket、Microsoft 等）
- Token 刷新（refresh token）和离线访问
- 连接器级别的组映射（group mapping）
- 自定义模板的登录页面
- 与 gangway/oauth2-proxy 配合实现 K8s 登录流程

## 使用场景与最佳实践

- Kubernetes 集群的统一身份认证网关
- 多集群场景下的联邦认证
- 企业 LDAP/AD 与 K8s RBAC 的桥接
- 开发环境的 GitHub OAuth 快速接入

## 参考链接

- https://dexidp.io/
- https://github.com/dexidp/dex

## Related

- [[domain-17-system-foundation/topic-dictionary/security/oauth2-proxy.md|oauth2-proxy]]
- [[domain-17-system-foundation/topic-dictionary/security/opa.md|OPA]]
- [[domain-17-system-foundation/topic-dictionary/security/vault.md|Vault]]
