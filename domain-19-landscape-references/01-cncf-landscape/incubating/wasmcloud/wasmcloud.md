---
title: wasmCloud (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- wasmcloud
- crd
- operator
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- wasmCloud 是什么
- 如何 wasmCloud
trigger_keywords:
- wasmCloud
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# wasmCloud

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Rust

## 概述

wasmCloud 是用于构建分布式 WebAssembly 应用的平台。它提供安全、可移植的应用运行环境，通过能力模型（Capability Model）实现组件与外部资源的解耦，支持跨云、边缘和本地的统一部署。

## 核心能力

- **WebAssembly 运行时**: 基于 wasmtime 的安全沙箱执行
- **能力模型**: 组件通过接口契约访问外部资源
- **位置透明**: 组件可在任意节点运行
- **热更新**: 无停机更新组件和配置
- **多语言支持**: Rust、Go、JavaScript、Python 等
- **分布式网络**: NATS 消息总线连接所有节点

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **组件轻量化**: 保持组件专注于业务逻辑，外部资源访问通过 Provider
- **版本管理**: 使用 OCI 镜像仓库管理组件版本
- **测试策略**: 使用 wash 测试工具验证组件行为
- **监控集成**: 配置 OTEL 导出追踪和指标
- **安全边界**: 利用 Wasm 沙箱和能力模型实现最小权限

## 架构定位

在 CNCF 生态中，wasmcloud 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/vault.md|[[HashiCorp Vault|vault]]]]
- [[deployment]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[ko]] — ko
- [[openfunction]] — OpenFunction
- [[kubevirt]] — KubeVirt
- [[nats]] — NATS
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 04-wasmcloud-platform
- wasmcloud
- [[entities/cncf-runtime|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
