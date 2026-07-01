---
title: Spin WASM 框架
description: Spin 是 Fermyon 开源的 WebAssembly 应用开发框架，支持用 Rust/Go/Python/JavaScript/TypeScript
  编...
summary: Spin 是 Fermyon 开源的 WebAssembly 应用开发框架，支持用 Rust/Go/Python/JavaScript/TypeScript
  编...
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- wasm
- serverless
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spin WASM 框架 是什么
- Spin 详解
trigger_keywords:
- Spin WASM 框架
- Spin
- dictionary
prerequisites:
- kubernetes
---



# Spin WASM 框架（Spin）

## 概述

Spin 是 Fermyon 开源的 WebAssembly 应用开发框架，支持用 Rust/Go/Python/JavaScript/TypeScript 编写 Serverless Wasm 应用，是 SpinKube 的底层开发框架。

## 核心概念/原理

- **多语言 SDK**：Rust/Go/Python/JS/TS 编写 Wasm 组件
- **Serverless 模型**：基于 HTTP/Redis 触发的函数执行
- **Fermyon 主导**：Wasm 应用平台的开源核心
- **组件模型**：基于 WebAssembly Component Model

## 关键机制或特性

- `spin new` 创建应用模板
- `spin build` 编译为 Wasm
- `spin up` 本地运行
- 支持 HTTP 触发器和 Redis 触发器
- KV/SQLite 内置存储
- SpinKube 部署到 K8s
- Fermyon Cloud 托管部署

## 使用场景与最佳实践

- Serverless API 的快速开发
- 边缘计算的函数运行时
- 微服务的 Wasm 化改造
- 多语言 Wasm 应用的统一开发
- 安全沙箱中的插件执行

## 参考链接

- https://www.fermyon.com/spin
- https://github.com/fermyon/spin

## Related

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/spinkube.md|SpinKube]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/wasmedge.md|WasmEdge]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative.md|Knative]]
