---
title: Paralus 访问控制
description: Paralus 是 CNCF Sandbox 项目，为 Kubernetes 提供集中式的访问控制和审计平台，支持 SSO、RBAC 和
  kubectl 访问代...
summary: Paralus 是 CNCF Sandbox 项目，为 Kubernetes 提供集中式的访问控制和审计平台，支持 SSO、RBAC 和 kubectl
  访问代...
category: dictionary
tags:
- k8s
- glossary
- security
- access-control
- multi-cluster
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Paralus 访问控制 是什么
- Paralus 详解
trigger_keywords:
- Paralus 访问控制
- Paralus
- dictionary
prerequisites:
- kubernetes
---



# Paralus 访问控制（Paralus）

## 概述

Paralus 是 CNCF Sandbox 项目，为 Kubernetes 提供集中式的访问控制和审计平台，支持 SSO、RBAC 和 kubectl 访问代理，是多集群权限管理的统一方案。

## 核心概念/原理

- **集中访问控制**：统一管理多集群的 K8s 访问权限
- **SSO 集成**：支持 OIDC/SAML/LDAP 身份源
- **CNCF Sandbox**：Rafay 主导
- **审计追踪**：完整的 kubectl 命令审计日志

## 关键机制或特性

- Zero Trust Access 代理（无需 VPN）
- 基于角色的 kubectl 访问控制
- 多集群的 RBAC 统一管理
- SSO 和 MFA 集成
- 命令审计和回放
- JIT（Just-in-Time）临时权限
- 用户/组/项目层次管理

## 使用场景与最佳实践

- 多集群的 K8s 权限集中管理
- 开发团队的 kubectl 安全访问
- 合规要求下的访问审计
- SSO 集成的统一认证
- 临时权限的安全分发

## 参考链接

- https://www.paralus.io/
- https://github.com/paralus/paralus

## Related

- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/security/keycloak.md|Keycloak]]
- [[domain-17-system-foundation/topic-dictionary/security/dex.md|Dex]]
