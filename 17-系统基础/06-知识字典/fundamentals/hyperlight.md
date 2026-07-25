---
title: Hyperlight 微虚拟机
description: Hyperlight 是微软开源的项目，提供超轻量的安全微虚拟机（microVM），专为 Wasm 和容器工作负载设计，在 Windows/Linux
  Hype...
summary: Hyperlight 是微软开源的项目，提供超轻量的安全微虚拟机（microVM），专为 Wasm 和容器工作负载设计，在 Windows/Linux
  Hype...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- microvm
- security
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hyperlight 微虚拟机 是什么
- Hyperlight 详解
trigger_keywords:
- Hyperlight 微虚拟机
- Hyperlight
- dictionary
prerequisites:
- kubernetes
---



# Hyperlight 微虚拟机（Hyperlight）

## 概述

Hyperlight 是微软开源的项目，提供超轻量的安全微虚拟机（microVM），专为 Wasm 和容器工作负载设计，在 Windows/Linux Hypervisor 上实现毫秒级启动和极低开销的隔离。

## 核心概念/原理

- **微虚拟机**：毫秒级启动的安全隔离环境
- **Hypervisor 驱动**：利用 Hyper-V/KVM 硬件虚拟化
- **微软开源**：Azure 基础设施的安全组件
- **Wasm 友好**：专为 Wasm 工作负载优化

## 关键机制或特性

- 基于 Hyper-V（Windows）/KVM（Linux）
- 轻量 Guest OS（<10MB 内存）
- 共享内存主机-Guest 通信
- Wasm 运行时集成（WasmEdge/Wasmtime）
- Rust 实现的安全 API
- 与 containerd shim 集成

## 使用场景与最佳实践

- Serverless 函数的安全隔离
- Wasm 工作负载的高性能沙箱
- 多租户环境的安全隔离
- 替代 Firecracker 的跨平台方案
- 边缘设备的轻量虚拟化

## 参考链接

- https://github.com/hyperlight-dev/hyperlight
- https://hyperlight.dev/

## Related

- [[17-系统基础/06-知识字典/fundamentals/kata-containers.md|Kata Containers]]
- [[17-系统基础/06-知识字典/fundamentals/runc.md|runc]]
- [[17-系统基础/06-知识字典/fundamentals/urunc.md|urunc]]
