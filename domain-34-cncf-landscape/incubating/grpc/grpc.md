---
title: gRPC
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- istio
- envoy
- hpa
- gateway
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- gRPC 是什么
- 如何 gRPC
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- gRPC
- cncf
- landscape
---


# gRPC

> **成熟度**: Incubating | **加入时间**: 2017-02 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://grpc.io |
| **GitHub** | https://github.com/grpc/grpc |
| **文档** | https://grpc.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | C++, Go, Java, Python 等 |
| **CNCF 分类** | RPC |

---

## 项目概述

### 简介
gRPC 是 Google 开源的高性能、通用的远程过程调用(RPC)框架。它基于 HTTP/2 协议传输和 Protocol Buffers 序列化，支持多种编程语言，是微服务架构中服务间通信的首选方案。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2015-02 | Google 开源 gRPC |
| 2017-02 | 加入 CNCF Incubating |
| 2017-09 | gRPC 1.0 发布 |
| 至今 | 被广泛用于云原生生态 |

### 核心定位
gRPC 是云原生服务间通信的标准，被 Kubernetes、Istio、Envoy、etcd 等核心项目使用，是构建高性能微服务的关键技术。

---

## 架构设计

### 通信模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    gRPC 四种通信模式                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Unary (一元调用)                                             │
│  ┌────────────┐    Request    ┌────────────┐                    │
│  │   Client   │──────────────►│   Server   │                    │
│  │            │◄──────────────│            │                    │
│  └────────────┘   Response    └────────────┘                    │
│                                                                  │
│  2. Server Streaming (服务端流)                                  │
│  ┌────────────┐    Request    ┌────────────┐                    │
│  │   Client   │──────────────►│   Server   │                    │
│  │            │◄──────────────│            │                    │
│  │            │◄──────────────│            │                    │
│  │            │◄──────────────│            │                    │
│  └────────────┘   Stream      └────────────┘                    │
│                                                                  │
│  3. Client Streaming (客户端流)                                  │
│  ┌────────────┐               ┌────────────┐                    │
│  │   Client   │──────────────►│   Server   │                    │
│  │            │──────────────►│            │                    │
│  │            │──────────────►│            │                    │
│  │            │◄──────────────│            │                    │
│  └────────────┘   Stream      └────────────┘                    │
│                   Response                                       │
│                                                                  │
│  4. Bidirectional Streaming (双向流)                             │
│  ┌────────────┐               ┌────────────┐                    │
│  │   Client   │◄─────────────►│   Server   │                    │
│  │            │◄─────────────►│            │                    │
│  │            │◄─────────────►│            │                    │
│  └────────────┘               └────────────┘                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### HTTP/2 优势

```
┌─────────────────────────────────────────────────────────────────┐
│                HTTP/1.1 vs HTTP/2                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  HTTP/1.1                        HTTP/2                         │
│  ┌──────────────────┐           ┌──────────────────┐            │
│  │ Request 1        │           │ Frame 1 (Req 1)  │            │
│  │ ────────────────►│           │ Frame 2 (Req 2)  │            │
│  │ Response 1       │           │ Frame 3 (Res 1)  │            │
│  │ ◄────────────────│           │ Frame 4 (Req 3)  │            │
│  │ Request 2        │ 顺序阻塞  │ Frame 5 (Res 2)  │ 多路复用   │
│  │ ────────────────►│           │ Frame 6 (Res 3)  │            │
│  │ Response 2       │           │        ...       │            │
│  │ ◄────────────────│           └──────────────────┘            │
│  └──────────────────┘                                           │
│                                                                  │
│  特性对比:                                                       │
│  • 连接复用: HTTP/1.1 需多连接, HTTP/2 单连接多路复用            │
│  • 头部压缩: HTTP/2 使用 HPACK 压缩                              │
│  • 二进制帧: HTTP/2 使用二进制分帧，更高效                       │
│  • 服务端推送: HTTP/2 支持服务端主动推送                         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Protocol Buffers

### 定义服务

```protobuf
// user.proto
syntax = "proto3";

package user;

option go_package = "github.com/example/user";

// 用户服务定义
service UserService {
  // 一元调用
  rpc GetUser(GetUserRequest) returns (User);
  
  // 服务端流
  rpc ListUsers(ListUsersRequest) returns (stream User);
  
  // 客户端流
  rpc CreateUsers(stream User) returns (CreateUsersResponse);
  
  // 双向流
  rpc Chat(stream ChatMessage) returns (stream ChatMessage);
}

// 消息定义
message User {
  int64 id = 1;
  string name = 2;
  string email = 3;
  repeated string roles = 4;
  google.protobuf.Timestamp created_at = 5;
}

message GetUserRequest {
  int64 id = 1;
}

message ListUsersRequest {
  int32 page_size = 1;
  string page_token = 2;
}

message CreateUsersResponse {
  int32 created_count = 1;
}

message ChatMessage {
  string user = 1;
  string content = 2;
}
```

### 生成代码

```bash
# Go
protoc --go_out=. --go-grpc_out=. user.proto

# Python
python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. user.proto

# Java
protoc --java_out=. --grpc-java_out=. user.proto
```

---

## 实现示例

### Go 服务端

```go
package main

import (
    "context"
    "log"
    "net"
    
    "google.golang.org/grpc"
    pb "github.com/example/user"
)

type userServer struct {
    pb.UnimplementedUserServiceServer
}

func (s *userServer) GetUser(ctx context.Context, req *pb.GetUserRequest) (*pb.User, error) {
    return &pb.User{
        Id:    req.Id,
        Name:  "John Doe",
        Email: "john@example.com",
    }, nil
}

func (s *userServer) ListUsers(req *pb.ListUsersRequest, stream pb.UserService_ListUsersServer) error {
    users := []*pb.User{
        {Id: 1, Name: "User 1"},
        {Id: 2, Name: "User 2"},
    }
    for _, user := range users {
        if err := stream.Send(user); err != nil {
            return err
        }
    }
    return nil
}

func main() {
    lis, _ := net.Listen("tcp", ":50051")
    
    s := grpc.NewServer()
    pb.RegisterUserServiceServer(s, &userServer{})
    
    log.Println("Server listening on :50051")
    s.Serve(lis)
}
```

### Go 客户端

```go
package main

import (
    "context"
    "io"
    "log"
    
    "google.golang.org/grpc"
    "google.golang.org/grpc/credentials/insecure"
    pb "github.com/example/user"
)

func main() {
    conn, _ := grpc.Dial("localhost:50051",
        grpc.WithTransportCredentials(insecure.NewCredentials()))
    defer conn.Close()
    
    client := pb.NewUserServiceClient(conn)
    
    // 一元调用
    user, _ := client.GetUser(context.Background(), &pb.GetUserRequest{Id: 1})
    log.Printf("User: %v", user)
    
    // 服务端流
    stream, _ := client.ListUsers(context.Background(), &pb.ListUsersRequest{})
    for {
        user, err := stream.Recv()
        if err == io.EOF {
            break
        }
        log.Printf("User: %v", user)
    }
}
```

---

## 高级特性

### 拦截器 (Interceptor)

```go
// 一元拦截器
func loggingUnaryInterceptor(ctx context.Context, req interface{},
    info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
    
    log.Printf("Method: %s, Request: %v", info.FullMethod, req)
    start := time.Now()
    resp, err := handler(ctx, req)
    log.Printf("Duration: %v, Error: %v", time.Since(start), err)
    return resp, err
}

// 应用拦截器
s := grpc.NewServer(
    grpc.UnaryInterceptor(loggingUnaryInterceptor),
    grpc.StreamInterceptor(loggingStreamInterceptor),
)
```

### 负载均衡

```go
// 客户端负载均衡
conn, _ := grpc.Dial(
    "dns:///my-service:50051",
    grpc.WithDefaultServiceConfig(`{"loadBalancingPolicy":"round_robin"}`),
)
```

### TLS/mTLS

```go
// 服务端 TLS
creds, _ := credentials.NewServerTLSFromFile("server.crt", "server.key")
s := grpc.NewServer(grpc.Creds(creds))

// 客户端 mTLS
creds, _ := credentials.NewClientTLSFromFile("ca.crt", "")
conn, _ := grpc.Dial("localhost:50051", grpc.WithTransportCredentials(creds))
```

---

## 生态集成

| 组件 | 用途 |
|:---|:---|
| **grpc-gateway** | gRPC 到 REST 代理 |
| **grpc-web** | 浏览器 gRPC 客户端 |
| **Envoy** | gRPC 负载均衡和代理 |
| **Istio** | 服务网格中的 gRPC 管理 |
| **OpenTelemetry** | gRPC 可观测性 |

---

## 参考资源

- [官方文档](https://grpc.io/docs)
- [GitHub Repo](https://github.com/grpc/grpc)
- [CNCF 项目页面](https://www.cncf.io/projects/grpc/)
- [Protocol Buffers](https://protobuf.dev/)
- [grpc-gateway](https://github.com/grpc-ecosystem/grpc-gateway)

---

**维护者**: Kudig Team | **许可证**: MIT
