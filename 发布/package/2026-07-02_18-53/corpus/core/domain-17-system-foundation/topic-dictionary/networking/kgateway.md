---
title: KGateway API 网关
description: KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于
  Envoy Proxy...
summary: KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于 Envoy
  Proxy...
category: dictionary
tags:
- k8s
- glossary
- networking
- gateway
- envoy
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KGateway API 网关 是什么
- KGateway 详解
trigger_keywords:
- KGateway API 网关
- KGateway
- dictionary
prerequisites:
- kubernetes
---



# KGateway API 网关（KGateway）

## 概述

KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于 Envoy Proxy，完整支持 Gateway API，提供丰富的流量管理和安全功能。

## 核心概念/原理

- **Envoy 驱动**：基于 Envoy 的高性能网关
- **Gateway API**：完整支持 Kubernetes Gateway API
- **多协议**：HTTP/gRPC/WebSocket/TCP
- **Solo.io**：企业级网关方案

## 关键机制或特性

- Gateway API 完整实现
- 路由规则和流量分割
- 速率限制和熔断
- TLS 终止和 mTLS
- WAF（Web Application Firewall）集成
- AI Gateway 功能（LLM 路由/Token 管理）
- 与 Grafana/Prometheus 可观测性集成

## 使用场景与最佳实践

- Kubernetes 入口流量管理
- API 网关和反向代理
- 微服务的统一入口
- Gateway API 的生产部署
- AI 应用的 API 网关

## 参考链接

- https://kgateway.dev/
- https://github.com/kgateway-dev/kgateway

## Related

- [[domain-17-system-foundation/知识字典/networking/envoy-gateway.md|Envoy Gateway]]
- [[domain-17-system-foundation/知识字典/networking/contour.md|Contour]]
- [[domain-17-system-foundation/知识字典/networking/traefik.md|Traefik]]
