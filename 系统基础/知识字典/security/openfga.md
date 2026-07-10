---
title: OpenFGA 授权引擎
description: OpenFGA 是 CNCF Sandbox 项目，高性能的关系型授权引擎，基于 Google Zanzibar 论文实现，为应用提供细粒度的权限检查（如
  'u...
summary: OpenFGA 是 CNCF Sandbox 项目，高性能的关系型授权引擎，基于 Google Zanzibar 论文实现，为应用提供细粒度的权限检查（如
  'u...
category: dictionary
tags:
- k8s
- glossary
- security
- authorization
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFGA 授权引擎 是什么
- OpenFGA 详解
trigger_keywords:
- OpenFGA 授权引擎
- OpenFGA
- dictionary
prerequisites:
- kubernetes
---



# OpenFGA 授权引擎（OpenFGA）

## 概述

OpenFGA 是 CNCF Sandbox 项目，高性能的关系型授权引擎，基于 Google Zanzibar 论文实现，为应用提供细粒度的权限检查（如 'user X can read document Y'）。

## 核心概念/原理

- **Zanzibar 实现**：基于 Google Zanzibar 的关系型授权模型
- **高性能**：微秒级权限检查延迟
- **CNCF Sandbox**：Okta/Auth0 主导
- **关系模型**：灵活的用户-对象-权限关系定义

## 关键机制或特性

- Authorization Model 定义权限关系
- Relationship Tuples 存储权限关系
- Check API 权限检查
- ListObjects API 列出可访问对象
- WriteAuthorizationModel 动态更新模型
- SDK（Go/JS/Python/Java/.NET）
- Playground 可视化调试

## 使用场景与最佳实践

- 应用的细粒度授权
- 文档/资源的权限管理
- SaaS 产品的多租户权限
- 社交网络的关注/好友关系
- 替代 RBAC/ABAC 的灵活授权方案

## 参考链接

- https://openfga.dev/
- https://github.com/openfga/openfga

## Related

- [[系统基础/知识字典/security/opa.md|OPA]]
- [[系统基础/知识字典/security/rbac.md|RBAC]]
- [[系统基础/知识字典/security/keycloak.md|Keycloak]]
