---
title: Service Mesh
description: Service Mesh 知识域 — Istio/Linkerd/Consul/Envoy 对比、Ambient Mesh、韧性模式、API 网关集成
category: subdomain
tags:
- istio
- linkerd
- envoy
- service-mesh
- ambient-mesh
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 服务网格 Service Mesh

> 微服务间通信的基础设施层，提供流量管理、安全、可观测性。

## Service Mesh 对比

| Mesh | 数据平面 | 特点 | 适用 |
|------|----------|------|------|
| Istio | Envoy | 功能最全、生态最大 | 企业级/复杂场景 |
| Linkerd | linkerd2-proxy | 轻量、低资源 | 简单微服务 |
| Consul Connect | Envoy/内置 | 多数据中心 | 混合云 |
| Cilium Mesh | eBPF | 无 Sidecar、内核级 | 高性能/低延迟 |
| Ambient Mesh | ztunnel+waypoint | 无 Sidecar Istio | Istio 轻量化 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[网络/服务网格/01-istio-enterprise-service-mesh.md\|Istio 企业级]] | 架构/部署/运维 | advanced |
| [[网络/服务网格/02-linkerd-enterprise-service-mesh.md\|Linkerd]] | 轻量级 Mesh 实践 | intermediate |
| [[网络/服务网格/03-consul-connect-enterprise.md\|Consul Connect]] | 多数据中心 Mesh | advanced |
| [[网络/服务网格/04-envoy-proxy-enterprise.md\|Envoy 代理]] | Envoy 深度配置 | advanced |
| [[网络/服务网格/05-dapr-enterprise-distributed-runtime.md\|Dapr]] | 分布式应用运行时 | intermediate |
| [[网络/服务网格/06-traefik-mesh-enterprise.md\|Traefik Mesh]] | 轻量 Mesh 方案 | intermediate |
| [[网络/服务网格/07-service-mesh-comparison-selection.md\|选型对比]] | Mesh 选型决策 | intermediate |
| [[网络/服务网格/08-ambient-mesh-l7-policy.md\|Ambient Mesh]] | 无 Sidecar L7 策略 | advanced |
| [[网络/服务网格/09-microservice-resilience-patterns.md\|韧性模式]] | 重试/熔断/超时/限流 | intermediate |
| [[网络/服务网格/10-api-gateway-service-mesh-integration.md\|网关集成]] | API Gateway + Mesh | advanced |
| [[网络/服务网格/99-istio-service-mesh-guide.md\|Istio 指南]] | 完整实践指南 | advanced |
| [[网络/服务网格/99-linkerd-service-mesh-guide.md\|Linkerd 指南]] | 完整实践指南 | intermediate |
| [[网络/服务网格/99-spring-cloud-kubernetes-service-mesh-guide.md\|Spring Cloud 指南]] | Java 微服务 Mesh | advanced |

## Related

- [[网络/eBPF/index.md|eBPF 网络]]
- [[网络/网络基础/index.md|网络基础]]
- [[应用模式/index.md|应用模式]]
