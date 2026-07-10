---
title: wasmCloud WASM 平台
description: wasmCloud 是 CNCF Sandbox 项目，提供基于 WebAssembly 的分布式应用运行时，通过 Actor 模型和能力接口（Capabili...
summary: wasmCloud 是 CNCF Sandbox 项目，提供基于 WebAssembly 的分布式应用运行时，通过 Actor 模型和能力接口（Capabili...
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- wasm
- distributed
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- wasmCloud WASM 平台 是什么
- wasmCloud 详解
trigger_keywords:
- wasmCloud WASM 平台
- wasmCloud
- dictionary
prerequisites:
- kubernetes
---



# wasmCloud WASM 平台（wasmCloud）

## 概述

wasmCloud 是 CNCF Sandbox 项目，提供基于 WebAssembly 的分布式应用运行时，通过 Actor 模型和能力接口（Capability）构建安全、可移植的分布式系统。

## 核心概念/原理

- **Actor 模型**：基于 Actor 的分布式应用架构
- **能力接口**：标准化的能力抽象（HTTP/KV/Messaging/Logging）
- **CNCF Sandbox**：Cosmonic 主导
- **安全沙箱**：Wasm 提供强隔离的执行环境

## 关键机制或特性

- wash CLI 开发工具
- Actor（组件）和 Provider（能力提供者）
- Lattice（分布式运行时网格）
- WIT（Wasm Interface Types）定义接口
- OCI Registry 分发组件
- 多语言支持（Rust/Go/TypeScript/Python）

## 使用场景与最佳实践

- 分布式微服务的 Wasm 化
- 跨云/边的可移植应用
- 安全隔离的插件架构
- IoT/边缘的分布式应用
- 能力驱动的组件化开发

## 参考链接

- https://wasmcloud.com/
- https://github.com/wasmCloud/wasmCloud

## Related

- [[系统基础/知识字典/specialized-workloads/spin.md|Spin]]
- [[系统基础/知识字典/fundamentals/wasmedge.md|WasmEdge]]
- [[系统基础/知识字典/platform-engineering/dapr.md|Dapr]]
