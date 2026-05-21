---
title: Connect RPC
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- connect-rpc
- gateway
- crd
- operator
last_updated: 2026-05
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

# Connect RPC

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, TypeScript, Swift, Kotlin

## 概述

Connect RPC 是一个轻量级、高性能的 RPC 框架，旨在简化 Protobuf 服务的开发和调用。它兼容 gRPC 协议的同时，支持标准 HTTP/1.1 和 HTTP/2，使得服务可以直接在浏览器中调用（无需代理）。Connect 提供 Go、TypeScript/JavaScript、Swift 和 Kotlin 的客户端和服务端实现，让开发者可以使用熟悉的 HTTP 语义构建...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **优先 Connect 协议**: 对浏览器客户端使用 Connect 协议，避免 gRPC-Web 代理
- **Buf 生态**: 使用 Buf 管理 Protobuf 依赖和代码生成
- **拦截器**: 通过拦截器实现认证、日志、指标等横切关注点
- **错误码**: 使用 Connect 标准错误码 (InvalidArgument, NotFound 等) 而非自定义
- **版本管理**: Protobuf 包使用版本号 (v1, v2)，便于 API 演进

## 架构定位

在 CNCF 生态中，connect-rpc 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[dex]] — Dex
- [[kgateway]] — kgateway
- [[urunc]] — urunc
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- [[domain-19-landscape-references/sandbox/connect-rpc/connect-rpc.md|connect-rpc]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
