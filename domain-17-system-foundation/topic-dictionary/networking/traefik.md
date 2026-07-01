---
title: Traefik
description: 'Traefik 是现代化的 HTTP 反向代理和负载均衡器，原生支持 Docker、Kubernetes、Consul 等多种后端。它作为 Kubernetes...'
category: dictionary
tags:
- k8s
- glossary
- traefik
- ingress
- reverse-proxy
- gateway-api
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Traefik 是什么
- Traefik 详解
trigger_keywords:
- Traefik
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Traefik

> **英文名**: Traefik

## 概述

Traefik 是现代化的 HTTP 反向代理和负载均衡器，原生支持 Docker、Kubernetes、Consul 等多种后端。它作为 Kubernetes Ingress Controller 和 Gateway API 实现，以自动服务发现和实时配置更新著称。

## 核心概念/原理

### 核心架构

- **EntryPoints**：入口点（HTTP/HTTPS/TCP 端口）。
- **Routers**：路由规则（匹配 Host/Path/Header）。
- **Services**：后端服务（负载均衡组）。
- **Middlewares**：请求处理链（认证、限流、重定向等）。
- **Providers**：配置源（K8s Ingress/Gateway API/Docker 等）。

### 与 Nginx Ingress 对比

| 特性 | Traefik | Nginx Ingress |
|------|---------|---------------|
| 配置更新 | 热更新（无 reload） | reload |
| Dashboard | 内置 | 无 |
| Gateway API | 原生支持 | 支持 |
| 中间件 | 丰富的 Middleware | Annotation |

## 关键机制或特性

- **自动服务发现**：监听 K8s API 自动发现 Ingress/Gateway 资源。
- **Middleware 链**：RateLimit、CircuitBreaker、Auth、Compress 等。
- **Let's Encrypt**：自动签发和续期 TLS 证书。
- **Dashboard**：内置 Web UI 查看路由和中间件状态。
- **TCP/UDP**：支持非 HTTP 协议的流量代理。

## 使用场景与最佳实践

- 中小集群可选择 Traefik 替代 Nginx Ingress Controller。
- 使用 Middleware 实现限流、认证、压缩等功能。
- 启用自动 TLS（Let's Encrypt）简化证书管理。
- 配合 Gateway API 实现更精细的流量管理。
- 使用 Traefik Pilot 或 Prometheus 监控代理指标。

## 参考链接

- [Traefik Official](https://doc.traefik.io/traefik/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate.md|Certificate]]
- [[domain-17-system-foundation/topic-dictionary/networking/loadbalancer.md|LoadBalancer]]
