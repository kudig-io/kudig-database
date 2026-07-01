---
title: gRPC
description: gRPC 是 Google 开源的高性能远程过程调用（RPC）框架，使用 Protocol Buffers 作为接口定义语言和数据序列化格式。它是微服务间通信的...
summary: gRPC 是 Google 开源的高性能远程过程调用（RPC）框架，使用 Protocol Buffers 作为接口定义语言和数据序列化格式。它是微服务间通信的...
category: dictionary
tags:
- k8s
- glossary
- grpc
- rpc
- protobuf
- networking
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gRPC 是什么
- gRPC 详解
trigger_keywords:
- gRPC
- dictionary
prerequisites:
- kubectl-basics
---



# gRPC

> **英文名**: gRPC

## 概述

gRPC 是 Google 开源的高性能远程过程调用（RPC）框架，使用 Protocol Buffers 作为接口定义语言和数据序列化格式。它是微服务间通信的主流方案之一，在 Kubernetes 生态中广泛用于控制平面和数据平面的内部通信。

## 核心概念/原理

### 核心概念

- **Protocol Buffers（protobuf）**：强类型的 IDL 和高效序列化格式。
- **四种通信模式**：

| 模式 | 说明 |
|------|------|
| Unary | 请求-响应 |
| Server Streaming | 服务端流式推送 |
| Client Streaming | 客户端流式上传 |
| Bidirectional Streaming | 双向流式通信 |

### 与 REST 对比

| 特性 | REST/JSON | gRPC/Protobuf |
|------|-----------|---------------|
| 序列化 | 文本（JSON） | 二进制（Protobuf） |
| 性能 | 较低 | 高 |
| 流式 | 不原生支持 | 原生支持 |
| 浏览器 | 直接支持 | 需要 gRPC-Web |

## 关键机制或特性

- Kubernetes 的 kubelet ↔ apiserver、etcd ↔ apiserver 等组件间通信大量使用 gRPC。
- Envoy 原生支持 gRPC 代理、负载均衡和重试。
- gRPC Health Check 协议用于服务健康检查。
- gRPC Reflection 支持运行时服务发现。
- gRPC-Gateway 自动生成 REST API 代理。

## 使用场景与最佳实践

- 微服务间内部通信优先使用 gRPC。
- 对外 API 使用 gRPC-Gateway 同时提供 REST 接口。
- 配置合理的超时和重试策略（gRPC retry policy）。
- 使用 grpcurl 工具调试 gRPC 服务。
- 配合 OpenTelemetry 实现 gRPC 调用的分布式追踪。

## 参考链接

- [gRPC Official](https://grpc.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/topic-dictionary/networking/istio.md|Istio]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kube-apiserver.md|Kube-apiserver]]
