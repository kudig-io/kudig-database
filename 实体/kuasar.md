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
last_updated: 2026-05
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

Kuasar 是一个统一的容器沙箱管理框架，支持在同一个节点上同时运行多种类型的沙箱（MicroVM、App Kernel、Wasm）。它重新设计了 containerd 的 Sandbox API，将沙箱管理逻辑从 shim 中分离出来，使得一个 Sandboxer 进程可以管理同类型的所有沙箱实例，大幅减少常驻进程数量和内存开销。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **沙箱类型选择**: 安全敏感用 MicroVM，高密度用 App Kernel，轻量函数用 Wasm
- **混合部署**: 在同一集群中通过 RuntimeClass 为不同工作负载选择合适的沙箱类型
- **资源规划**: MicroVM 需要更多内存开销，合理规划节点容量
- **监控**: 监控 Sandboxer 进程的资源使用和沙箱创建延迟
- **升级策略**: Sandboxer 管理多个沙箱，升级时需要 drain 节点

## 架构定位

在 CNCF 生态中，kuasar 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

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
