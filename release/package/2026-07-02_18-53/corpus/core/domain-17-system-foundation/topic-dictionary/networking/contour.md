---
title: Contour Ingress 控制器
description: Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress
  和 Gatew...
summary: Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress
  和 Gatew...
category: dictionary
tags:
- k8s
- glossary
- networking
- ingress
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
- Contour Ingress 控制器 是什么
- Contour 详解
trigger_keywords:
- Contour Ingress 控制器
- Contour
- dictionary
prerequisites:
- kubernetes
---



# Contour Ingress 控制器（Contour）

## 概述

Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress 和 Gateway API，提供高性能的 L7 负载均衡和流量管理能力。

## 核心概念/原理

- **Envoy 驱动**：使用 Envoy 作为数据面，控制面用 Go 编写
- **双 API 支持**：同时支持 Kubernetes Ingress 和 Gateway API
- **HTTProxy CRD**：Contour 自定义的路由配置资源，支持丰富的流量策略
- **CNCF Sandbox**：CNCF 沙箱项目

## 关键机制或特性

- 动态 Envoy 配置（通过 xDS API）
- TLS 终止与 SNI 路由
- 流量分割（权重路由）用于金丝雀发布
- WebSocket / gRPC 代理
- 速率限制（集成 ratelimit 服务）
- Contour 支持多 Gateway 部署

## 使用场景与最佳实践

- 替代 nginx-ingress 的高性能 Ingress 方案
- 需要 Envoy 级别流量控制的场景
- Gateway API 的早期采纳
- 金丝雀发布和流量镜像需求

## 参考链接

- https://projectcontour.io/
- https://github.com/projectcontour/contour

## Related

- [[domain-17-system-foundation/知识字典/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/知识字典/networking/envoy-gateway.md|Envoy Gateway]]
- [[domain-17-system-foundation/知识字典/networking/traefik.md|Traefik]]
