---
title: Envoy
description: Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo
  等云原生项目的数据平...
summary: Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo 等云原生项目的数据平...
category: dictionary
tags:
- k8s
- glossary
- envoy
- service-mesh
- proxy
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
- Envoy 是什么
- Envoy Proxy 详解
trigger_keywords:
- Envoy
- Envoy Proxy
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Envoy

> **英文名**: Envoy Proxy

## 概述

Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo 等云原生项目的数据平面基础，广泛用于服务网格、API 网关和入口控制器场景。

## 核心概念/原理

### 核心概念

- **Listener**：监听入站连接的端口/地址。
- **Filter Chain**：处理连接的过滤器链（认证、限流、路由等）。
- **Cluster**：上游服务集群（后端端点集合）。
- **Route**：路由规则，将请求映射到 Cluster。

### xDS API

Envoy 通过 xDS（发现服务 API）动态获取配置：

| xDS | 用途 |
|-----|------|
| LDS | Listener 发现 |
| RDS | Route 发现 |
| CDS | Cluster 发现 |
| EDS | Endpoint 发现 |
| SDS | Secret 发现 |

## 关键机制或特性

- **Sidecar 模式**：作为 Pod 的 sidecar 容器运行（Istio 默认）。
- **Gateway 模式**：作为入口/出口网关运行。
- 支持 HTTP/1.1、HTTP/2、gRPC、TCP、UDP 协议。
- 内置熔断、重试、超时、限流等弹性功能。
- 支持 Wasm 扩展自定义过滤器。

## 使用场景与最佳实践

- 使用 Envoy 作为 API Gateway 的数据平面（Gateway API 支持）。
- 配合 Istio 构建服务网格实现 mTLS 和流量管理。
- 使用 Envoy Admin API（`/config_dump`、`/stats`）排查问题。
- 监控 Envoy 的 upstream_rq_time 和 upstream_cx_connect_fail 指标。
- 合理配置 Circuit Breaker 防止级联故障。

## 参考链接

- [Envoy Proxy Official](https://www.envoyproxy.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/networkpolicy.md|NetworkPolicy]]
- [[domain-17-system-foundation/topic-dictionary/networking/cilium.md|Cilium]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
