---
title: container2wasm 容器转换
description: container2wasm 是 containerd 作者之一 Kazuyoshi Kato 开源的工具，将 Linux 容器镜像转换为
  WebAssembl...
summary: container2wasm 是 containerd 作者之一 Kazuyoshi Kato 开源的工具，将 Linux 容器镜像转换为 WebAssembl...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- wasm
- container
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- container2wasm 容器转换 是什么
- container2wasm 详解
trigger_keywords:
- container2wasm 容器转换
- container2wasm
- dictionary
prerequisites:
- kubernetes
---



# container2wasm 容器转换（container2wasm）

## 概述

container2wasm 是 containerd 作者之一 Kazuyoshi Kato 开源的工具，将 Linux 容器镜像转换为 WebAssembly 模块（WASI），使容器可以在 Wasm 运行时（浏览器/边缘设备）中运行。

## 核心概念/原理

- **容器转 Wasm**：将 OCI 镜像转换为 .wasm 文件
- **Linux 模拟**：通过 Wasm 模拟 Linux 系统调用
- **广泛运行**：转换后可在浏览器/Wasm 运行时中运行
- **containerd 生态**：与 containerd 深度集成

## 关键机制或特性

- `ctr-remote` 转换工具
- 支持 amd64/arm64 容器镜像
- WASI Preview 1 输出
- 与 WasmEdge/Wasmtime/Wasmer 兼容
- 转换后的镜像可在浏览器中运行
- 文件系统打包（ext4 in Wasm）

## 使用场景与最佳实践

- 容器工作负载的边缘部署
- 浏览器中的容器应用演示
- Wasm 运行时的容器兼容性
- 安全沙箱中的容器执行
- 跨架构的容器运行

## 参考链接

- https://github.com/ktock/container2wasm
- https://ktock.medium.com/

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/wasmedge.md|WasmEdge]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker.md|Docker]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd.md|containerd]]
