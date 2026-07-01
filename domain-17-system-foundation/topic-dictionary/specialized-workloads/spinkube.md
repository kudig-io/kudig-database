---
title: SpinKube WASM 运行时
description: 'SpinKube 是 Fermyon 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Spin WebAssembly 应用，通过 R...'
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- wasm
- serverless
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SpinKube WASM 运行时 是什么
- SpinKube 详解
trigger_keywords:
- SpinKube WASM 运行时
- SpinKube
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# SpinKube WASM 运行时（SpinKube）

## 概述

SpinKube 是 Fermyon 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Spin WebAssembly 应用，通过 RuntimeClass 将 Wasm 工作负载与容器工作负载统一调度。

## 核心概念/原理

- **Wasm on K8s**：在 K8s 上原生运行 WebAssembly 组件
- **Spin 框架**：基于 Spin SDK 的 Serverless Wasm 应用
- **CNCF Sandbox**：Fermyon 主导
- **RuntimeClass**：通过 Kwasm 运行时类集成

## 关键机制或特性

- SpinApp CRD 定义 Wasm 应用
- 基于 Spin SDK 的多语言支持（Rust/Go/Python/JS）
- Kwasm Operator 自动安装 Wasm runtime
- 毫秒级冷启动
- 与 K8s Service/Ingress 集成
- 资源占用极低（KB 级内存）

## 使用场景与最佳实践

- Serverless 函数的 Wasm 运行时
- 边缘计算的超轻量工作负载
- 安全隔离的插件执行环境
- 多语言微服务的统一运行时
- 快速启动的 API 网关和中间件

## 参考链接

- https://www.spinkube.dev/
- https://github.com/spinkube/spin-operator

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/kata-containers.md|Kata Containers]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative.md|Knative]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/openfaas.md|OpenFaaS]]
