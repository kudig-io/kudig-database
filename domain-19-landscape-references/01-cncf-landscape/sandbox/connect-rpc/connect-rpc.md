---
title: Connect RPC
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Connect RPC 是什么
- 如何 Connect RPC
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Connect
- RPC
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: Connect RPC
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Connect RPC 是什么
- 如何 Connect RPC
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Connect
- RPC
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Connect RPC

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://connectrpc.com/ |
| **GitHub** | https://github.com/connectrpc/connect-go |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, TypeScript, Swift, Kotlin |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Connect RPC 是一个轻量级、高性能的 RPC 框架，旨在简化 Protobuf 服务的开发和调用。它兼容 gRPC 协议的同时，支持标准 HTTP/1.1 和 HTTP/2，使得服务可以直接在浏览器中调用（无需代理）。Connect 提供 Go、TypeScript/JavaScript、Swift 和 Kotlin 的客户端和服务端实现，让开发者可以使用熟悉的 HTTP 语义构建类型安全的 API。

### 核心特性

- **三协议支持**: 同时支持 Connect 协议、gRPC 协议和 gRPC-Web 协议
- **浏览器原生**: Connect 协议使用标准 HTTP，浏览器可直接调用无需代理
- **类型安全**: 基于 Protobuf，提供编译时类型检查和代码生成
- **简洁 API**: 比传统 gRPC 更简洁的服务端和客户端 API
- **流式支持**: 完整支持 unary、server streaming、client streaming 和 bidirectional streaming
- **可观测性**: 内置拦截器支持日志、指标和链路追踪

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                      Clients                           │
│                                                       │
│  ┌─────────────┐ ┌──────────────┐ ┌───────────────┐  │
│  │ Browser     │ │ Mobile App   │ │ Backend       │  │
│  │ (JS/TS)     │ │ (Swift/      │ │ Service       │  │
│  │             │ │  Kotlin)     │ │ (Go/TS)       │  │
│  └──────┬──────┘ └──────┬───────┘ └───────┬───────┘  │
│         │               │                  │          │
│         │ Connect       │ Connect/gRPC     │ gRPC     │
│         │ Protocol      │ Protocol         │ Protocol │
└─────────┼───────────────┼──────────────────┼──────────┘
          │               │                  │
          └───────────────┼──────────────────┘
                          │
               ┌──────────▼──────────┐
               │   Connect Server     │
               │   (Go/Node.js)       │
               │                      │
               │  ┌────────────────┐  │
               │  │ Protocol       │  │
               │  │ Handlers       │  │
               │  │ ┌───────────┐  │  │
               │  │ │ Connect   │  │  │
               │  │ │ gRPC      │  │  │
               │  │ │ gRPC-Web  │  │  │
               │  │ └───────────┘  │  │
               │  └────────────────┘  │
               │                      │
               │  ┌────────────────┐  │
               │  │ Service Impl   │  │
               │  │ (Your Code)    │  │
               │  └────────────────┘  │
               └──────────────────────┘
```

---

## 快速开始

### 定义 Protobuf 服务

```protobuf
// greet/v1/greet.proto
syntax = "proto3";

package greet.v1;

option go_package = "example/gen/greet/v1;greetv1";

message GreetRequest {
  string name = 1;
}

message GreetResponse {
  string greeting = 1;
}

service GreetService {
  rpc Greet(GreetRequest) returns (GreetResponse);
}
```

### 生成代码

```bash
# 安装 buf 和插件
go install github.com/bufbuild/buf/cmd/buf@latest
go install connectrpc.com/connect/cmd/protoc-gen-connect-go@latest

# buf.gen.yaml
version: v1
plugins:
  - plugin: go
    out: gen
    opt: paths=source_relative
  - plugin: connect-go
    out: gen
    opt: paths=source_relative

# 生成代码
buf generate
```

### Go 服务端实现

```go
package main

import (
    "context"
    "net/http"

    "connectrpc.com/connect"
    greetv1 "example/gen/greet/v1"
    "example/gen/greet/v1/greetv1connect"
)

type GreetServer struct{}

func (s *GreetServer) Greet(
    ctx context.Context,
    req *connect.Request[greetv1.GreetRequest],
) (*connect.Response[greetv1.GreetResponse], error) {
    greeting := "Hello, " + req.Msg.Name + "!"
    return connect.NewResponse(&greetv1.GreetResponse{
        Greeting: greeting,
    }), nil
}

func main() {
    greeter := &GreetServer{}
    mux := http.NewServeMux()
    
    path, handler := greetv1connect.NewGreetServiceHandler(greeter)
    mux.Handle(path, handler)
    
    http.ListenAndServe(":8080", mux)
}
```

### Go 客户端调用

```go
package main

import (
    "context"
    "log"
    "net/http"

    "connectrpc.com/connect"
    greetv1 "example/gen/greet/v1"
    "example/gen/greet/v1/greetv1connect"
)

func main() {
    client := greetv1connect.NewGreetServiceClient(
        http.DefaultClient,
        "http://localhost:8080",
    )
    
    res, err := client.Greet(
        context.Background(),
        connect.NewRequest(&greetv1.GreetRequest{Name: "World"}),
    )
    if err != nil {
        log.Fatal(err)
    }
    log.Println(res.Msg.Greeting) // "Hello, World!"
}
```

### TypeScript 浏览器调用

```typescript
import { createPromiseClient } from "@connectrpc/connect";
import { createConnectTransport } from "@connectrpc/connect-web";
import { GreetService } from "./gen/greet/v1/greet_connect";

const transport = createConnectTransport({
  baseUrl: "http://localhost:8080",
});

const client = createPromiseClient(GreetService, transport);

async function greet() {
  const res = await client.greet({ name: "World" });
  console.log(res.greeting); // "Hello, World!"
}
```

---

## 高级功能

### 服务端流式

```protobuf
// 服务定义
service ChatService {
  rpc ServerStream(StreamRequest) returns (stream StreamResponse);
}
```

```go
func (s *ChatServer) ServerStream(
    ctx context.Context,
    req *connect.Request[chatv1.StreamRequest],
    stream *connect.ServerStream[chatv1.StreamResponse],
) error {
    for i := 0; i < 10; i++ {
        if err := stream.Send(&chatv1.StreamResponse{
            Message: fmt.Sprintf("Message %d", i),
        }); err != nil {
            return err
        }
    }
    return nil
}
```

### 拦截器 (Interceptor)

```go
// 日志拦截器
func loggingInterceptor() connect.UnaryInterceptorFunc {
    return func(next connect.UnaryFunc) connect.UnaryFunc {
        return func(ctx context.Context, req connect.AnyRequest) (connect.AnyResponse, error) {
            log.Printf("Request: %s", req.Spec().Procedure)
            start := time.Now()
            res, err := next(ctx, req)
            log.Printf("Duration: %v, Error: %v", time.Since(start), err)
            return res, err
        }
    }
}

// 使用拦截器
interceptors := connect.WithInterceptors(loggingInterceptor())
_, handler := greetv1connect.NewGreetServiceHandler(greeter, interceptors)
```

### 错误处理

```go
import "connectrpc.com/connect"

func (s *GreetServer) Greet(
    ctx context.Context,
    req *connect.Request[greetv1.GreetRequest],
) (*connect.Response[greetv1.GreetResponse], error) {
    if req.Msg.Name == "" {
        return nil, connect.NewError(
            connect.CodeInvalidArgument,
            errors.New("name is required"),
        )
    }
    // ...
}
```

---

## 与其他方案对比

| 特性 | Connect RPC | gRPC | Twirp | REST/JSON |
|:---|:---|:---|:---|:---|
| 浏览器原生 | 直接支持 | 需代理 | 支持 | 原生 |
| 协议 | Connect/gRPC/gRPC-Web | gRPC | Twirp/JSON | HTTP |
| 流式 | 完整支持 | 完整支持 | 不支持 | SSE/WS |
| 类型安全 | Protobuf | Protobuf | Protobuf | OpenAPI |
| 代码生成 | 轻量 | 重量 | 轻量 | 可选 |
| 可读性 | HTTP 语义 | 二进制 | JSON | JSON |

---

## 最佳实践

1. **优先 Connect 协议**: 对浏览器客户端使用 Connect 协议，避免 gRPC-Web 代理
2. **Buf 生态**: 使用 Buf 管理 Protobuf 依赖和代码生成
3. **拦截器**: 通过拦截器实现认证、日志、指标等横切关注点
4. **错误码**: 使用 Connect 标准错误码 (InvalidArgument, NotFound 等) 而非自定义
5. **版本管理**: Protobuf 包使用版本号 (v1, v2)，便于 API 演进

---

## 参考资源

- [Connect RPC 官方文档](https://connectrpc.com/docs/)
- [connect-go GitHub](https://github.com/connectrpc/connect-go)
- [connect-es (TypeScript)](https://github.com/connectrpc/connect-es)
- [Buf 工具链](https://buf.build/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/connect-rpc.md|Connect RPC]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
