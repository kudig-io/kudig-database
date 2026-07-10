---
title: Kuasar 多沙箱运行时
description: Kuasar 是华为开源的 CNCF Sandbox 项目，提供多沙箱容器运行时管理，统一 containerd 与多种沙箱运行时（Kata/microVM/W...
summary: Kuasar 是华为开源的 CNCF Sandbox 项目，提供多沙箱容器运行时管理，统一 containerd 与多种沙箱运行时（Kata/microVM/W...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- container-runtime
- sandbox
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuasar 多沙箱运行时 是什么
- Kuasar 详解
trigger_keywords:
- Kuasar 多沙箱运行时
- Kuasar
- dictionary
prerequisites:
- kubernetes
---



# Kuasar 多沙箱运行时（Kuasar）

## 概述

Kuasar 是华为开源的 CNCF Sandbox 项目，提供多沙箱容器运行时管理，统一 containerd 与多种沙箱运行时（Kata/microVM/Wasm/AppKernel）的集成，简化异构运行时的部署和管理。

## 核心概念/原理

- **多沙箱统一**：一套 sandboxer 管理多种沙箱类型
- **containerd 集成**：通过 Sandboxer API 与 containerd 无缝对接
- **CNCF Sandbox**：华为开源，社区活跃
- **异构支持**：Kata Containers、WasmEdge、Quark、gVisor 等

## 关键机制或特性

- Sandboxer 插件架构（每种沙箱一个 sandboxer 实现）
- 通过 containerd runtime handler 选择沙箱类型
- 支持 Kata/microVM/Wasm/AppKernel 四种沙箱
- 统一的沙箱生命周期管理
- 轻量级管理进程，资源开销低
- 与 Kubernetes RuntimeClass 集成

## 使用场景与最佳实践

- 需要多种容器运行时共存的集群
- Kata Containers + Wasm 混合工作负载
- 安全隔离要求不同的混合工作负载
- 边缘设备的异构运行时管理
- containerd 生态的运行时扩展

## 参考链接

- https://kuasar.io/
- https://github.com/kuasar-io/kuasar

## Related

- [[系统基础/topic-dictionary/fundamentals/runc.md|runc]]
- [[系统基础/topic-dictionary/fundamentals/kata-containers.md|Kata Containers]]
- [[系统基础/topic-dictionary/fundamentals/youki.md|youki]]
