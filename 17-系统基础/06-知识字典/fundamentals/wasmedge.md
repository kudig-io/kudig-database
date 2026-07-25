---
title: WasmEdge WASM 运行时
description: WasmEdge 是 Second State 开源的 CNCF Sandbox 项目，高性能 WebAssembly 运行时，专为云原生和边缘计算优化，支持
  ...
summary: WasmEdge 是 Second State 开源的 CNCF Sandbox 项目，高性能 WebAssembly 运行时，专为云原生和边缘计算优化，支持
  ...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- wasm
- runtime
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- WasmEdge WASM 运行时 是什么
- WasmEdge 详解
trigger_keywords:
- WasmEdge WASM 运行时
- WasmEdge
- dictionary
prerequisites:
- kubernetes
---



# WasmEdge WASM 运行时（WasmEdge）

## 概述

WasmEdge 是 Second State 开源的 CNCF Sandbox 项目，高性能 WebAssembly 运行时，专为云原生和边缘计算优化，支持 AI 推理、网络服务和嵌入式设备的 Wasm 执行。

## 核心概念/原理

- **高性能 WASM**：JIT 编译执行，接近原生性能
- **AI 推理**：内置 TensorFlow/PyTorch/ONNX 等 AI 推理扩展
- **CNCF Sandbox**：Second State 主导
- **多场景**：云/边/端统一的 Wasm 运行时

## 关键机制或特性

- 支持 WASI（WebAssembly System Interface）
- 网络 Socket（WASI-NN/WASI-Socket）
- AI 推理插件（TensorFlow Lite/PyTorch/Whisper）
- Kubernetes RuntimeClass 集成
- 支持 JavaScript/Python/Rust Wasm 模块
- AOT（Ahead-of-Time）编译优化

## 使用场景与最佳实践

- Serverless 函数的 Wasm 运行时
- AI 推理服务的边缘部署
- 微服务的轻量级运行时
- 插件系统的安全沙箱
- Kubernetes 上的 Wasm 工作负载

## 参考链接

- https://wasmedge.org/
- https://github.com/WasmEdge/WasmEdge

## Related

- [[17-系统基础/06-知识字典/fundamentals/runc.md|runc]]
- [[17-系统基础/06-知识字典/specialized-workloads/spinkube.md|SpinKube]]
- [[17-系统基础/06-知识字典/fundamentals/kuasar.md|Kuasar]]
