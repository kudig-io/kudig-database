---
title: Connect RPC [entities]
description: '## 概述'
summary: 'Connect RPC 是一个轻量级、高性能的 RPC 框架，旨在简化 Protobuf 服务的开发和调用。'
category: entities
tags:
- k8s
- cncf
- networking
- connect-rpc
- gateway
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Connect RPC 是什么
- 如何 Connect RPC
trigger_keywords:
- Connect
- RPC
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Connect RPC

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, TypeScript, Swift, Kotlin

## 概述

Connect RPC（简称 Connect）是一个轻量级、高性能的 RPC 框架，由 Buf 开发，2023 年加入 CNCF 沙箱。它旨在简化 Protobuf 服务的开发和调用，兼容 gRPC 协议的同时，支持标准 HTTP/1.1 和 HTTP/2，使得服务可以直接在浏览器中调用（无需 gRPC-Web 代理）。Connect 提供 Go、TypeScript/JavaScript、Swift 和 Kotlin 的客户端和服务端实现，让开发者可以使用熟悉的 HTTP 语义构建 gRPC 兼容的 API。与 gRPC 相比，Connect 的最大优势是"协议三合一"——同一套代码同时支持 Connect 协议（HTTP+JSON）、gRPC 和 gRPC-Web，前端可以直连后端服务，无需 Envoy 等代理层。

## 核心能力

- **三协议兼容**: 同一套服务同时支持 Connect、gRPC、gRPC-Web 协议
- **浏览器直连**: 前端可直接调用 Connect 服务，无需 gRPC-Web 代理
- **HTTP/1.1 支持**: 支持标准 HTTP/1.1（gRPC 仅支持 HTTP/2）
- **Protobuf 原生**: 基于Protobuf IDL 定义服务，自动生成多语言代码
- **流式支持**: 支持 unary、server-streaming、client-streaming、bidirectional-streaming
- **Buf 生态集成**: 与 Buf（Protobuf 工具链）深度集成

## 架构

Connect RPC 采用协议适配 + Protobuf 序列化设计：

- **Connect Protocol**: 基于 HTTP 的 RPC 协议，使用 Protobuf 或 JSON 序列化
- **gRPC 兼容层**: 自动处理 gRPC wire format（frame + protobuf）
- **gRPC-Web 兼容层**: 自动处理 gRPC-Web（base64 + HTTP/1.1）
- **Interceptor**: 请求/响应拦截器（认证、日志、指标、重试）
- **Codec**: 可插拔的序列化编解码器（Protobuf/JSON）
- **Code Generator**: 基于 Protobuf 插件生成多语言客户端和服务端代码

请求流程：`客户端 (Connect/gRPC/gRPC-Web) → Connect Handler → 业务逻辑 → 响应`

## K8s 集成

Connect RPC 服务作为标准 HTTP/gRPC 服务运行在 Kubernetes Pod 中。由于 Connect 同时支持 HTTP/1.1 和 HTTP/2，任何标准 Ingress Controller（nginx、envoy、traefik）都可以直接代理 Connect 服务，无需特殊的 gRPC 配置。前端应用通过 Kubernetes Ingress 直接调用 Connect API，无需额外的 gRPC-Web 代理。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Service、Ingress 和 Gateway API 完全兼容。

## 生产场景

1. **全栈 Protobuf API**: 前端（TypeScript）和后端（Go）共用 Protobuf 定义
2. **浏览器直连后端**: 前端通过 Connect 协议直连后端，省去 API 网关
3. **gRPC 兼容迁移**: 从 gRPC 迁移到 Connect，获得 HTTP/1.1 和浏览器支持
4. **移动端 API**: iOS（Swift）和 Android（Kotlin）客户端直连 Connect 服务

## 安装

```bash
# Go 服务端示例
go get connectrpc.com/connect@latest
go get connectrpc.com/bufconnect@latest

# 生成 Connect 代码（需要 protoc + buf）
buf generate

# Go 服务端
cat > main.go <<'GO'
package main

import (
    "net/http"
    "connectrpc.com/connect"
    "gen/ping/v1/pingconnect"
)

type PingServer struct{}

func (s *PingServer) Ping(ctx context.Context, req *connect.Request[pingv1.PingRequest]) (*connect.Response[pingv1.PingResponse], error) {
    return connect.NewResponse(&pingv1.PingResponse{Msg: "pong"}), nil
}

func main() {
    mux := http.NewServeMux()
    pingv1.RegisterPingServiceHandler(mux, &PingServer{})
    http.ListenAndServe(":8080", mux)
}
GO

# TypeScript 客户端（浏览器直连）
# import { createClient } from "@connectrpc/connect";
# import { createGrpcWebTransport } from "@connectrpc/connect-web";
# const client = createClient(PingService, createGrpcWebTransport({ baseUrl: "https://api.example.com" }));
# const res = await client.ping({ name: "world" });
```

## 对比

| 特性 | Connect | gRPC | gRPC-Web | Twirp |
|------|---------|------|----------|-------|
| 浏览器直连 | ✅ | ❌ | ⚠️ 需代理 | ✅ |
| HTTP/1.1 | ✅ | ❌ | ❌ | ✅ |
| gRPC 兼容 | ✅ | ✅ | ⚠️ | ❌ |
| 流式 | ✅ | ✅ | ⚠️ 有限 | ❌ |

## 架构定位

在 CNCF 生态中，Connect 属于 **Networking** 类别，为云原生应用提供轻量级高性能 RPC 能力。

## 参考链接

- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[dex]] — Dex
- [[kgateway]] — kgateway
- [[urunc]] — urunc
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- connect-rpc
- [[实体/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference


<!-- risk-assessed -->
