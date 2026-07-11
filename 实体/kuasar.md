---
title: Kuasar (entities)
description: '## 概述'
summary: 'Kuasar 是一个统一的容器沙箱管理框架，支持在同一个节点上同时运行多种类型的沙箱（MicroVM、App Kernel、Wasm）。它重新设计了 containerd 的 Sandbox API，将沙箱管理逻辑从 shim 中分离出来，使得一个 Sandboxer 进程可以管理同类型的所有沙箱实例，大幅减少常驻进程数量和内存开销。'
category: entities
tags:
- k8s
- cncf
- runtime
- kuasar
- containerd
- crd
- operator
- wasm
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuasar 是什么
- 如何 Kuasar
trigger_keywords:
- Kuasar
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kuasar

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Kuasar 是一个 CNCF 沙箱项目，由华为开源，是一个高性能的容器沙箱运行时。它利用 Linux 内核的多种沙箱技术（VM、MicroVM、WASM、Kontain）为容器提供高效的隔离方案。Kuasar 作为 containerd 的沙箱 API 实现，支持多种沙箱后端，同时保持极低的资源开销和启动延迟。与 Kata Containers 类似但架构更轻量，特别适合 Serverless 和多租户场景。

## Key Features（核心能力）

- **多沙箱后端**：支持 MicroVM（Cloud Hypervisor/StratoVirt）、WASM、Kontain 等隔离方案
- **低开销**：沙箱管理器共享，不每个 Pod 启动独立 shim 进程
- **Sandbox API 原生**：基于 containerd 2.0 Sandbox API 设计
- **快速启动**：MicroVM 启动时间在 100ms 以内
- **Rust 实现**：内存安全和高性能的 Rust 核心引擎
- **多种容器格式**：支持 OCI 容器和 WASM 模块

## 架构与工作原理

Kuasar 架构基于 containerd Sandboxed API。每个沙箱由一个 Kuasar-shim 进程管理（而非每个 Pod 一个 shim），大幅减少资源开销。shim 通过 VMM（Virtual Machine Monitor）接口管理底层沙箱实例——MicroVM 后端使用 Cloud Hypervisor 或 StratoVirt，WASM 后端使用 WasmEdge，Kontain 后端使用 Kontain Runtime。所有沙箱共享 containerd 的镜像管理。

## K8s 集成

Kuasar 通过 RuntimeClass 与 Kubernetes 集成。在 K8s 节点上安装 Kuasar 并配置 containerd 使用 Kuasar 作为沙箱运行时。创建 RuntimeClass（如 kuasar-vmm）指定 handler 为 kuasar。Pod 通过 runtimeClassName: kuasar-vmm 选择使用 Kuasar MicroVM 沙箱。与 K8s Device Plugin 集成支持设备直通。

## 生产用例

- **Serverless 平台**：为 FaaS 提供快速启动和强隔离的沙箱环境
- **多租户安全**：利用 MicroVM 提供接近硬件级的隔离
- **遗留应用容器化**：需要完整 OS 内核的遗留应用安全运行
- **合规环境**：满足金融/医疗等对隔离性的严格要求

## 安装与快速开始

```bash
# 安装 Kuasar
cargo install kuasar
# 配置 containerd
# /etc/containerd/config.toml
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kuasar]
#   runtime_type = "io.containerd.kuasar.v1"
```

## 对比替代方案

相比 Kata Containers（每个 Pod 一个 shim+QEMU），Kuasar 通过共享 shim 大幅减少资源开销。相比 gVisor（用户态内核），Kuasar 的 MicroVM 后端提供更好的兼容性。

## Related

- [[cozystack]] — Cozystack
- [[fluid]] — Fluid
- storage.md|cncf-storage]] — CNCF 存储与数据库项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- kuasar
- [[实体/urunc.md|[[urunc (Unikernel Container Runtime)|urunc]]]]
- [[实体/hyperlight.md|Hyperlight]]
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
