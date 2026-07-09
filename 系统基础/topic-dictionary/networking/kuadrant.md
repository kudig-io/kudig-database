---
title: Kuadrant API 管理
description: Kuadrant 是 Red Hat 开源的 CNCF Sandbox 项目，基于 Gateway API 提供 API 管理能力（认证/授权/限流），为
  Ku...
summary: Kuadrant 是 Red Hat 开源的 CNCF Sandbox 项目，基于 Gateway API 提供 API 管理能力（认证/授权/限流），为
  Ku...
category: dictionary
tags:
- k8s
- glossary
- networking
- api-management
- gateway
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuadrant API 管理 是什么
- Kuadrant 详解
trigger_keywords:
- Kuadrant API 管理
- Kuadrant
- dictionary
prerequisites:
- kubernetes
---



# Kuadrant API 管理（Kuadrant）

## 概述

Kuadrant 是 Red Hat 开源的 CNCF Sandbox 项目，基于 Gateway API 提供 API 管理能力（认证/授权/限流），为 Kubernetes API 网关添加策略层。

## 核心概念/原理

- **Gateway API 增强**：为 K8s Gateway 添加策略管理
- **CNCF Sandbox**：Red Hat 主导
- **策略层**：认证/授权/限流/速率控制
- **多网关**：兼容 Envoy Gateway/Istio 等

## 关键机制或特性

- AuthPolicy（认证和授权策略）
- RateLimitPolicy（速率限制策略）
- DNSPolicy（DNS 管理）
- TLSPolicy（TLS 管理）
- 与 Gateway API 无缝集成
- OPA 策略引擎后端
- 多网关供应商支持

## 使用场景与最佳实践

- API 网关的策略管理
- 微服务的认证和授权
- API 限流和保护
- Gateway API 的企业增强
- 多网关的统一策略管理

## 参考链接

- https://kuadrant.io/
- https://github.com/Kuadrant/kuadrant-operator

## Related

- [[系统基础/topic-dictionary/networking/envoy-gateway.md|Envoy Gateway]]
- [[系统基础/topic-dictionary/networking/kgateway.md|KGateway]]
- [[系统基础/topic-dictionary/security/openfga.md|OpenFGA]]
