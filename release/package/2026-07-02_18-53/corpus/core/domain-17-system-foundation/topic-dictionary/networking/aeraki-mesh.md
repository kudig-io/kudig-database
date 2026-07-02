---
title: Aeraki Mesh 七层网格
description: Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩...
summary: Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩...
category: dictionary
tags:
- k8s
- glossary
- networking
- service-mesh
- l7
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Aeraki Mesh 七层网格 是什么
- Aeraki Mesh 详解
trigger_keywords:
- Aeraki Mesh 七层网格
- Aeraki Mesh
- dictionary
prerequisites:
- kubernetes
---



# Aeraki Mesh 七层网格（Aeraki Mesh）

## 概述

Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩展到 TCP 和任意七层协议（Dubbo、Thrift、Redis 等）。

## 核心概念/原理

- **协议扩展**：将 Istio 的流量管理扩展到任意 L7 协议
- **Dubbo 支持**：完整支持 Apache Dubbo 协议的流量治理
- **Redis 支持**：Redis 协议的流量镜像、故障注入等
- **腾讯开源**：基于腾讯大规模微服务实践

## 关键机制或特性

- Aeraki Protocol Framework 协议扩展框架
- 支持 Dubbo、Thrift、Redis、MySQL 等非 HTTP 协议
- Aeraki Mesh CRD 定义七层路由规则
- 与 Istio 控制面无缝集成
- MetaProtocol 元协议框架（协议无关的流量治理）
- LazyXDS 按需加载优化大规模集群性能

## 使用场景与最佳实践

- 使用 Dubbo/Thrift 等传统 RPC 框架的微服务网格化
- 需要非 HTTP 协议流量治理的场景
- Istio 生态的协议扩展
- 传统微服务向服务网格迁移
- 多协议混合环境的统一管理

## 参考链接

- https://www.aeraki.net/
- https://github.com/aeraki-mesh/aeraki

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/istio.md|Istio]]
- [[domain-17-system-foundation/topic-dictionary/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/topic-dictionary/networking/linkerd.md|Linkerd]]
