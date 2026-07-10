---
title: Athenz 身份认证与授权
description: Athenz 是 Yahoo 开源并捐赠给 CNCF 的服务平台，提供基于 X.509 证书的服务身份认证和细粒度角色授权（RBAC），专为大规模微服务和云原生...
summary: Athenz 是 Yahoo 开源并捐赠给 CNCF 的服务平台，提供基于 X.509 证书的服务身份认证和细粒度角色授权（RBAC），专为大规模微服务和云原生...
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- authorization
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Athenz 身份认证与授权 是什么
- Athenz 详解
trigger_keywords:
- Athenz 身份认证与授权
- Athenz
- dictionary
prerequisites:
- kubernetes
---



# Athenz 身份认证与授权（Athenz）

## 概述

Athenz 是 Yahoo 开源并捐赠给 CNCF 的服务平台，提供基于 X.509 证书的服务身份认证和细粒度角色授权（RBAC），专为大规模微服务和云原生环境设计。

## 核心概念/原理

- **双功能**：同时提供服务身份认证（Service Authentication）和角色授权（Authorization）
- **X.509 短证书**：自动签发和轮转短期服务身份证书，零信任架构基础
- **集中策略管理**：中心化管理跨服务的访问策略
- **大规模验证**：Yahoo 生产环境支撑数十万服务实例

## 关键机制或特性

- ZMS（Athenz Management Service）：策略和域名管理
- ZTS（Athenz Token Service）：Token 和证书签发
- 支持 Kubernetes Workload Identity 集成
- Athenz 域名模型：`<domain>.<service>` 命名体系
- REST API 和 CLI 管理工具

## 使用场景与最佳实践

- 大规模微服务间的 mTLS 身份认证
- 跨组织的服务访问授权管理
- 零信任网络中的服务身份基础设施
- 多云/混合云环境的统一身份层

## 参考链接

- https://www.athenz.io/
- https://github.com/AthenZ/athenz

## Related

- [[系统基础/知识字典/security/spiffe-spire-identity.md|SPIFFE/SPIRE]]
- [[系统基础/知识字典/operations/cert-manager.md|cert-manager]]
- [[系统基础/知识字典/security/rbac.md|RBAC]]
