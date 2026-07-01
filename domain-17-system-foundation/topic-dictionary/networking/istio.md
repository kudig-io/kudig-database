---
title: Istio
description: 'Istio 是最广泛使用的开源服务网格平台，为微服务通信提供流量管理、安全（mTLS）、可观测性和策略执行能力。它使用 Envoy 作为数据平面代理，通过控制平...'
category: dictionary
tags:
- k8s
- glossary
- istio
- service-mesh
- envoy
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Istio 是什么
- Istio 详解
trigger_keywords:
- Istio
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Istio

> **英文名**: Istio

## 概述

Istio 是最广泛使用的开源服务网格平台，为微服务通信提供流量管理、安全（mTLS）、可观测性和策略执行能力。它使用 Envoy 作为数据平面代理，通过控制平面（istiod）统一管理配置。

## 核心概念/原理

### 核心架构

- **istiod**：控制平面，合并了原来的 Pilot、Galley、Citadel。
- **Envoy Sidecar**：自动注入到每个 Pod 的数据平面代理。
- **Istio Gateway**：集群入口/出口流量管理。

### 流量管理原语

| 资源 | 功能 |
|------|------|
| VirtualService | 路由规则（权重、header 匹配等） |
| DestinationRule | 上游策略（负载均衡、熔断、子集） |
| Gateway | 入口/出口 L4-L6 配置 |
| ServiceEntry | 网格外部服务声明 |

## 关键机制或特性

- **mTLS**：自动为服务间通信启用双向 TLS 加密。
- **流量拆分**：通过 VirtualService 实现金丝雀发布和 A/B 测试。
- **故障注入**：模拟延迟和错误，验证服务韧性。
- **可观测性**：自动生成分布式追踪、指标和访问日志。
- Istio Ambient Mesh：无 sidecar 的新模式，降低资源开销。

## 使用场景与最佳实践

- 新集群评估是否需要服务网格（非所有场景都需要 Istio）。
- 使用 STRICT mTLS 模式确保所有服务间通信加密。
- 合理配置 DestinationRule 的 ConnectionPool 和 OutlierDetection。
- 使用 Kiali 可视化服务网格拓扑和流量。
- 关注 Istio Ambient Mesh 的发展，减少 sidecar 开销。

## 参考链接

- [Istio Official Documentation](https://istio.io/latest/docs/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/topic-dictionary/networking/cilium.md|Cilium]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate.md|Certificate]]
