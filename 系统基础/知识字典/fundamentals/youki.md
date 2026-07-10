---
title: youki 容器运行时
description: youki 是用 Rust 编写的 OCI 容器运行时，兼容 runc 接口，旨在提供更高安全性和性能的低开销容器运行时实现，是 runc
  的 Rust 替代方...
summary: youki 是用 Rust 编写的 OCI 容器运行时，兼容 runc 接口，旨在提供更高安全性和性能的低开销容器运行时实现，是 runc 的 Rust
  替代方...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- container-runtime
- rust
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- youki 容器运行时 是什么
- youki 详解
trigger_keywords:
- youki 容器运行时
- youki
- dictionary
prerequisites:
- kubernetes
---



# youki 容器运行时（youki）

## 概述

youki 是用 Rust 编写的 OCI 容器运行时，兼容 runc 接口，旨在提供更高安全性和性能的低开销容器运行时实现，是 runc 的 Rust 替代方案。

## 核心概念/原理

- **Rust 实现**：利用 Rust 的内存安全特性减少运行时漏洞
- **OCI 兼容**：完整实现 OCI Runtime Specification
- **runc 替代**：可直接替换 runc 使用
- **社区驱动**：containers 组织下的活跃开源项目

## 关键机制或特性

- 兼容 CRI-O 和 containerd 的 runtime shim
- 支持 cgroups v1 和 v2
- seccomp 和 capabilities 安全策略
- Rootless 模式支持
- 性能与 runc 相当，内存占用更低
- WasmEdge 集成支持 WebAssembly 工作负载

## 使用场景与最佳实践

- 需要更高安全保证的容器运行时
- runc 的替代方案评估
- 边缘设备的低开销容器运行
- Rust 生态的容器基础设施
- 安全合规要求严格的环境

## 参考链接

- https://github.com/containers/youki
- https://youki-dev.github.io/youki/

## Related

- [[系统基础/知识字典/fundamentals/runc.md|runc]]
- [[系统基础/知识字典/fundamentals/kata-containers.md|Kata Containers]]
- [[系统基础/知识字典/fundamentals/containerd.md|containerd]]
