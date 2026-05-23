---
title: OAuth2 Proxy [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- oauth2-proxy
- prometheus
- grafana
- redis
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OAuth2 Proxy 是什么
- 如何 OAuth2 Proxy
trigger_keywords:
- OAuth2
- Proxy
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- redis-basics
created: "2026-05-23"
---

# OAuth2 Proxy

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

OAuth2 Proxy 是一个反向代理，提供基于 OAuth2/OIDC 协议的身份认证功能。它充当应用程序前的认证网关，支持 Google、GitHub、Azure AD、Keycloak 等多种身份提供商（IdP），使后端应用无需自行实现认证逻辑即可获得统一的身份验证能力。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Cookie 安全**: 生产环境启用 `cookie-secure=true` 和 `cookie-httponly=true`
- **会话存储**: 多副本部署使用 Redis 共享会话，避免 Cookie-only 的大小限制
- **PKCE**: 启用 `code-challenge-method=S256` 增强授权码流安全性
- **令牌传递**: 使用 `set-xauthrequest=true` 让上游获取用户信息
- **组授权**: 使用 `allowed-group` 实现基于组的细粒度访问控制
- **监控告警**: 监控 `oauth2_proxy_auth_success_total` 异常变化检测潜在攻击

## 架构定位

在 CNCF 生态中，oauth2-proxy 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[drasi]] — Drasi
- [[containerssh]] — ContainerSSH
- [[modelpack]] — ModelPack
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[keycloak]] — Keycloak

- oauth2-proxy
- [[entities/cncf-security|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
