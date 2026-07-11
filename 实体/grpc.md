---
title: gRPC (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- networking
- grpc
- etcd
- istio
- crd
- operator
- argocd
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gRPC 是什么
- 如何 gRPC
trigger_keywords:
- gRPC
prerequisites:
- kubectl-basics
- service-mesh-basics
- gitops-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# gRPC

> **CNCF 状态**: Incubating | **类别**: Networking | **主要语言**: C++, Go, Java, Python 等

## 概述

gRPC 是一个 CNCF 孵化项目，由 Google 开源，是高性能的远程过程调用（RPC）框架。它基于 HTTP/2 和 Protocol Buffers，支持双向流式通信、强类型接口定义和跨语言互操作。gRPC 已成为云原生微服务通信的事实标准，被 Netflix、Square、Cisco、Slack 等数万家公司采用。在 Kubernetes 生态中，gRPC 被广泛用于服务间通信、API Server 扩展（Aggregated API Server）和 CSI/CRI/CNI 接口。

## Key Features（核心能力）

- **Protocol Buffers**：基于 IDL（.proto 文件）定义强类型接口，自动生成多语言代码
- **HTTP/2 传输**：利用 HTTP/2 多路复用、流式传输和头部压缩实现高效通信
- **四种流模式**：Unary、Server Streaming、Client Streaming、Bidirectional Streaming
- **跨语言支持**：官方支持 C++, Go, Java, Python, Node.js, C#, PHP 等 10+ 语言
- **健康检查**：内置 gRPC Health Checking Protocol 标准化服务健康检测
- **拦截器**：支持客户端和服务端拦截器实现认证、日志、指标等横切关注点

## 架构与工作原理

gRPC 架构基于 Protocol Buffers IDL 和 HTTP/2 协议。开发者通过 .proto 文件定义服务接口和消息类型，protoc 编译器生成客户端 Stub 和服务端骨架代码。运行时，gRPC Core（C 核心库）处理 HTTP/2 连接管理、流复用和 Protobuf 序列化/反序列化。上层语言绑定封装 gRPC Core 提供语言原生的 API。gRPC 还定义了标准的服务发现、健康检查、负载均衡等扩展协议。

## K8s 集成

在 Kubernetes 中，gRPC 广泛用于多个层面：API Server 扩展（Aggregated API Server、Admission Webhook）；CRI/CSI/CNI 接口通信；etcd 与 API Server 通信。对于 gRPC 微服务部署，需要特别注意负载均衡——gRPC 基于 HTTP/2 长连接，K8s 默认的 iptables 模式 Service 在连接建立后不会重新负载均衡。解决方案包括使用 client-side load balancing、gRPC xDS（Envoy）或 headless service + DNS 轮询。

## 生产用例

- **微服务间通信**：高性能低延迟的服务间 RPC 调用
- **流式数据处理**：实时数据流的 Server/Client Streaming 模式
- **API 网关后端**：gRPC-JSON 转换网关为前端提供 RESTful API
- **移动端 API**：高效的 Protobuf 编码节省移动网络带宽

## 安装与快速开始

```bash
# Go
protoc --go_out=. --go-grpc_out=. hello.proto
# Python
pip install grpcio grpcio-tools
python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. hello.proto
```

## 对比替代方案

相比 REST/JSON，gRPC 提供更强的类型安全、更低的延迟和更高的吞吐。相比 Thrift（Apache），gRPC 基于 HTTP/2 生态更丰富。相比 Connect-RPC（Buf），gRPC 更成熟但 Connect 对 HTTP/1.1 友好。

## Related

- [[46-terway-performance-tuning]] — Terway 性能调优
- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- grpc
- [[实体/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[实体/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/argocd.md|ArgoCD]] — Cross-reference
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
