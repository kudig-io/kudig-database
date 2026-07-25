---
title: Connect RPC 协议
description: Connect 是 Buf 开发的开源 RPC 协议，兼容 gRPC 和 Protobuf，但使用标准 HTTP 语义（HTTP/1.1
  和 HTTP/2），简...
summary: Connect 是 Buf 开发的开源 RPC 协议，兼容 gRPC 和 Protobuf，但使用标准 HTTP 语义（HTTP/1.1 和 HTTP/2），简...
category: dictionary
tags:
- k8s
- glossary
- networking
- rpc
- protocol
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Connect RPC 协议 是什么
- Connect RPC 详解
trigger_keywords:
- Connect RPC 协议
- Connect RPC
- dictionary
prerequisites:
- kubernetes
---



# Connect RPC 协议（Connect RPC）

## 概述

Connect 是 Buf 开发的开源 RPC 协议，兼容 gRPC 和 Protobuf，但使用标准 HTTP 语义（HTTP/1.1 和 HTTP/2），简化了浏览器和移动端的 API 调用，是 gRPC-Web 的现代替代方案。

## 核心概念/原理

- **协议兼容**：兼容 gRPC 和 gRPC-Web 的 wire format
- **HTTP 语义**：基于标准 HTTP 方法，无需特殊的 gRPC 代理
- **浏览器友好**：原生支持浏览器端调用，无需 grpc-web proxy
- **多语言 SDK**：支持 Go、TypeScript、Swift、Kotlin、Java 等

## 关键机制或特性

- 自动代码生成（基于 protobuf schema）
- Connect-Go / Connect-ES / Connect-Swift 等语言实现
- 支持 Unary、Server Streaming、Client Streaming、Bidi Streaming
- 与 Envoy / Nginx 等代理无缝集成
- 错误处理标准化（Connect Protocol Error）
- connectrpc 命令行工具

## 使用场景与最佳实践

- gRPC API 的浏览器端调用
- 移动应用的 RPC 通信
- 替代 REST 的类型安全 API
- gRPC-Web 的现代化升级
- 微服务间的高性能 RPC 通信

## 参考链接

- https://connectrpc.com/
- https://github.com/connectrpc/connect-go

## Related

- [[17-系统基础/06-知识字典/platform-engineering/grpc.md|gRPC]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
