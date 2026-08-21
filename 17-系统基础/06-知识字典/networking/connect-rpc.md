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

## 架构深度解析

### 协议分层

```
┌──────────────────────────────────────────────────────┐
│                  业务应用层（业务代码）                  │
├──────────────────────────────────────────────────────┤
│            Connect SDK（Connect-Go / ES / Swift）      │
│      ┌───────────────┐   ┌───────────────────┐       │
│      │ Unary 请求     │   │ Streaming 请求     │       │
│      │ (POST /svc/M)  │   │ (POST /svc/M:stream)│      │
│      └───────┬───────┘   └─────────┬─────────┘       │
├──────────────┼─────────────────────┼──────────────────┤
│    HTTP/1.1 协议（content-type: application/proto）   │
│    HTTP/2 协议（gRPC wire format 直连兼容）            │
├──────────────┼─────────────────────┼──────────────────┤
│    Envoy / Nginx / Cloud Gateway 等标准代理（无需专有） │
└──────────────┴─────────────────────┴──────────────────┘
```

### 三种协议模式

| 模式 | Content-Type | 场景 | 特点 |
|------|-------------|------|------|
| Connect Protocol | `application/proto` | 浏览器/移动端 | 标准 HTTP POST，GET 支持缓存 |
| gRPC Protocol | `application/grpc` | 服务间高性能 | 完全 gRPC wire 兼容 |
| gRPC-Web Protocol | `application/grpc-web` | 遗留 grpc-web 客户端 | 向后兼容 |

### 源码关键路径（connectrpc/connect-go）

| 模块 | 路径 | 职责 |
|------|------|------|
| 协议实现 | `protocol/` | Connect/gRPC/gRPC-Web 三种 wire format 编解码 |
| HTTP 适配 | `http_client.go` / `http_server.go` | 基于 net/http 的请求处理 |
| 拦截器 | `interceptor.go` | 客户端/服务端拦截器链 |
| 错误处理 | `error.go` | Connect Error 标准化错误模型 |
| 流式处理 | `stream.go` / `streaming_handler.go` | 三种流式模式实现 |

### 请求处理流程

1. 客户端 SDK 将 Protobuf 消息按所选协议编码（默认 Connect Protocol）
2. 通过标准 HTTP 发送：Unary 用 `POST /package.Service/Method`，消息体为二进制 proto
3. 服务端 net/http handler 按 Content-Type 分派到对应协议处理器
4. 解码请求、执行拦截器链、调用业务方法、编码响应
5. 错误统一返回 `application/json` 的 Connect Error（code + message + details）

## 生产案例

### 案例 1：移动端升级 Connect 后请求全部超时

| 时间 | 事件 |
|------|------|
| 14:00 | 移动端发版切换到 Connect SDK，用户反馈接口全部超时 |
| 14:05 | 服务端无任何请求日志，网关层也没有流量 |
| 14:10 | 抓包发现客户端请求携带 `Accept-Encoding: gzip`，服务端未处理 |
| 14:15 | 确认是 iOS 网络库对自定义 Content-Type 的 gzip 拦截问题 |
| 14:30 | 客户端禁用压缩或服务端显式处理 gzip，问题解决 |

**根因**：Connect 默认使用 `application/proto` Content-Type，部分平台网络栈会对其自动 gzip；服务端未配置解压导致请求体为空。

**修复命令**：
```bash
# 服务端启用 gzip 解压（Go 中间件示例）🟡 中风险
go get github.com/klauspost/compress/gzhttp
# 客户端禁用自动压缩 🟢 低风险
client := connect.NewClient[pingv1.PingRequest, pingv1.PingResponse](
    http.DefaultClient,
    "https://api.example.com",
    connect.WithSendCompression(connect.CompressionIdentity),
)
```

### 案例 2：gRPC 存量服务平滑迁移 Connect

**现象**：存量 gRPC 服务（Java）无法直接接入 Connect 客户端，担心双协议并存复杂度。

**诊断**：Connect 服务端天然同时暴露 Connect + gRPC 两种协议，同一 handler 无需改动；Java 端 gRPC 客户端可直接访问（wire 兼容）。

**修复**：接入侧只需新增 Connect 客户端接入层，存量 gRPC 调用零改动；用 `buf curl` 做双协议验证：
```bash
# 验证 gRPC 协议 🟢 只读
buf curl --protocol grpc https://api.example.com/ping.v1.PingService/Ping -d '{}'
# 验证 Connect 协议 🟢 只读
buf curl --protocol connect https://api.example.com/ping.v1.PingService/Ping -d '{}'
```

## 对比评测

| 维度 | Connect | gRPC-Web | REST + OpenAPI |
|------|---------|----------|----------------|
| 浏览器原生 | ✅ 直接支持 | 需 Envoy 代理 | ✅ |
| 流式支持 | 全双工流 | 仅服务端流 | ❌ |
| 类型安全 | Protobuf 强类型 | Protobuf | 运行时校验 |
| 代理兼容 | 标准 HTTP，任意代理 | 需 gRPC-Web 专用代理 | 标准 |
| 学习成本 | 低（HTTP 语义） | 中 | 低 |

**选型建议**：新项目推荐 Connect（浏览器 + 服务端一协议通吃）；纯服务端内部调用 gRPC 仍是性能最优；对外 API 若需人类可读文档则考虑 Connect + OpenAPI 生成。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 请求 415 | `buf curl --verbose` 查 Content-Type | 客户端协议模式与服务端不匹配 |
| 中文乱码/字段丢失 | 检查 proto 字段 JSON 命名 | 使用 `connect.WithJSON()` 需配 `useProtoNames` |
| 流中断 | 服务端日志查 EOF | 代理缓冲（如 Nginx 需关 proxy_buffering） |
| 404 | 检查路由注册 | 服务名/方法名大小写不匹配 |

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 生产流量切换到 Connect 后大面积超时/失败 | 回滚客户端版本，保留 gRPC 协议通道 |
| P1 | 存量 gRPC 服务计划统一协议栈 | 渐进式接入，双协议并存运行观察 1-2 个版本周期 |
| P2 | gRPC-Web 存量客户端维护成本高 | 规划迁移 Connect，同步下线 grpc-web 代理 |

## 面试要点

> 以下 Q&A 覆盖 Connect 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Connect 与 gRPC 在传输层到底有什么区别？**
   A：gRPC 依赖 HTTP/2 的专有帧语义（如 `grpc-status` trailer），必须走 HTTP/2；Connect 把 RPC 建模为普通 HTTP 请求（Unary 即 POST + 完整响应体），因此 HTTP/1.1 与 HTTP/2 都可用，无需 HTTP/2 专用代理即可穿越任意网关/CDN，这是浏览器兼容性的根本原因。

2. **Q：Connect 三种协议模式如何自动协商？**
   A：由客户端发送的 `Content-Type` 决定：`application/proto` → Connect 模式、`application/grpc` → gRPC 模式、`application/grpc-web` → grpc-web 模式。同一服务端可同时服务三种客户端，实现协议透明演进。

3. **Q：Connect 的流式（Streaming）与 gRPC 有何差异？**
   A：语义一致（Server/Client/Bidi Streaming），但 wire format 不同：Connect Streaming 基于 HTTP/1.1 chunked 或 HTTP/2 帧，每条消息有独立的长度前缀；gRPC 则使用 HTTP/2 DATA 帧 + 5 字节压缩标志。Connect 的 Bidi 流在 HTTP/1.1 下受限（需 HTTP/2 才能全双工）。

## 参考链接

- https://connectrpc.com/
- https://github.com/connectrpc/connect-go

## Related

- [[17-系统基础/06-知识字典/platform-engineering/grpc.md|gRPC]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
