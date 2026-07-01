---
title: Easegress 流量编排
description: 'Easegress 是 MegaEase 开源的 CNCF Sandbox 项目，提供全场景的流量编排能力，集 API 网关、服务网格 Sidecar、Serv...'
category: dictionary
tags:
- k8s
- glossary
- networking
- gateway
- service-mesh
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Easegress 流量编排 是什么
- Easegress 详解
trigger_keywords:
- Easegress 流量编排
- Easegress
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Easegress 流量编排（Easegress）

## 概述

Easegress 是 MegaEase 开源的 CNCF Sandbox 项目，提供全场景的流量编排能力，集 API 网关、服务网格 Sidecar、Service Mesh Controller 于一体，支持 HTTP/TCP/MQTT 等多协议。

## 核心概念/原理

- **全场景**：API Gateway + Service Mesh + Serverless Runtime
- **多协议**：HTTP/2、gRPC、WebSocket、MQTT、TCP
- **CNCF Sandbox**：MegaEase 主导
- **Go 编写**：高性能低资源占用

## 关键机制或特性

- Pipeline 流量处理管道
- Filter 链式过滤器（限流/认证/重试/路由等）
- 服务注册与发现（K8s/Consul/Eureka/Nacos）
- 分布式一致性（基于 Raft）
- Serverless Runtime（Wasm + 函数运行时）
- Prometheus 指标导出

## 使用场景与最佳实践

- API 网关和反向代理
- 微服务的流量治理
- MQTT IoT 设备流量管理
- Serverless 函数的网关层
- 传统系统现代化改造的流量层

## 参考链接

- https://megaease.com/easegress/
- https://github.com/megaease/easegress

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/traefik.md|Traefik]]
- [[domain-17-system-foundation/topic-dictionary/networking/envoy-gateway.md|Envoy Gateway]]
- [[domain-17-system-foundation/topic-dictionary/networking/contour.md|Contour]]
