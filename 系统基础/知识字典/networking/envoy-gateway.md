---
title: Envoy Gateway
description: Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy
  官方的网关方案，...
summary: Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy 官方的网关方案，...
category: dictionary
tags:
- k8s
- glossary
- envoy-gateway
- gateway-api
- ingress
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Envoy Gateway 是什么
- Envoy Gateway 详解
trigger_keywords:
- Envoy Gateway
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Envoy Gateway

> **英文名**: Envoy Gateway

## 概述

Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy 官方的网关方案，将 Envoy 作为独立的数据平面，通过 Gateway API 标准化管理入站流量。

## 核心概念/原理

### 核心架构

- **Envoy Gateway Controller**：监听 Gateway API 资源并配置 Envoy。
- **Envoy Proxy**：数据平面，处理实际流量。
- **EnvoyProxy CRD**：自定义 Envoy 部署和配置。

### Gateway API 概念

| 资源 | 功能 |
|------|------|
| GatewayClass | 网关实现类型 |
| Gateway | 入口点和监听器定义 |
| HTTPRoute | HTTP 路由规则 |
| TLSRoute | TLS 路由规则 |
| GRPCRoute | gRPC 路由规则 |

## 关键机制或特性

- **Gateway API 原生**：完全遵循 Kubernetes Gateway API 标准。
- **Envoy Extension**：支持 Envoy 的 Wasm/Lua 扩展。
- **Rate Limiting**：内置限流功能。
- **Security Policy**：JWT 验证、CORS、ExtAuth 等。
- **Traffic Splitting**：基于权重的流量分割（金丝雀）。

## 使用场景与最佳实践

- 新集群使用 Envoy Gateway 替代传统 Ingress Controller。
- 使用 Gateway API 标准化入站流量管理。
- 配合 cert-manager 自动管理 TLS 证书。
- 使用 EnvoyProxy CRD 自定义 Envoy 部署参数。
- 关注 Gateway API 的 GAMMA 倡议（服务间流量管理）。

## 参考链接

- [Envoy Gateway Official](https://gateway.envoyproxy.io/)

## Related

- [[系统基础/知识字典/networking/envoy.md|Envoy]]
- [[系统基础/知识字典/networking/ingress.md|Ingress]]
- [[系统基础/知识字典/networking/traefik.md|Traefik]]
- [[系统基础/知识字典/networking/istio.md|Istio]]
- [[系统基础/知识字典/security/certificate.md|Certificate]]


<!-- risk-assessed -->
