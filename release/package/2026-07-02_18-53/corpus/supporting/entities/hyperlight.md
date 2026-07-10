---
title: Hyperlight (entities)
description: '## 概述'
summary: 'Hyperlight 是一个轻量级虚拟机管理器 (VMM)，专为在毫秒级启动时间内运行函数式工作负载而设计。它创建超轻量的 micro-VM，每个 VM 可在 1-2 毫秒内启动，内存开销仅为几 MB。Hyperlight 特别适合 Serverless 和 FaaS 场景，提供比容器更强的隔离性，同时保持接近容器的启动速度和资源效率。'
category: entities
tags:
- k8s
- cncf
- runtime
- hyperlight
- argocd
- containerd
- falco
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hyperlight 是什么
- 如何 Hyperlight
trigger_keywords:
- Hyperlight
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Hyperlight

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Hyperlight 是一个轻量级虚拟机管理器 (VMM)，专为在毫秒级启动时间内运行函数式工作负载而设计。它创建超轻量的 micro-VM，每个 VM 可在 1-2 毫秒内启动，内存开销仅为几 MB。Hyperlight 特别适合 Serverless 和 FaaS 场景，提供比容器更强的隔离性，同时保持接近容器的启动速度和资源效率。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Sandbox 池**: 对于高并发场景，使用 SandboxPool 复用 VM 实例减少创建开销
- **最小内存**: 根据 Guest 实际需求配置最小内存，提高部署密度
- **超时保护**: 为所有 Guest 调用设置超时，防止恶意或异常 Guest 阻塞
- **无状态 Guest**: 设计无状态的 Guest 函数，便于 Sandbox 复用
- **Host 函数最小化**: 减少 Host 函数暴露面，降低安全风险

## 架构定位

在 CNCF 生态中，hyperlight 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/argocd.md|[[ArgoCD|argocd]]]]

## Related

- [[falco]] — Falco
- [[operator-framework]] — Operator Framework
- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hyperlight
- [[entities/urunc.md|[[urunc (Unikernel Container Runtime)|urunc]]]]
- [[entities/flatcar.md|Flatcar Container Linux]]
- [[entities/composefs.md|composefs]]
- [[entities/04-containerd-upgrade-migration.md|containerd 升级迁移]]
- [[entities/wasmedge.md|WasmEdge]]
- [[entities/spinkube.md|SpinKube]]
- [[entities/05-containerd-windows-support.md|containerd Windows 支持]]
- [[entities/02-containerd-v2-features.md|containerd 2.0 新特性]]
- [[entities/08-containerd-multi-tenant.md|containerd 多租户]]
- [[entities/k0s.md|K0s]]
- [[entities/03-containerd-security-hardening.md|containerd 安全加固]]
- [[entities/bootc.md|bootc]]
- [[entities/container2wasm.md|container2wasm]]
- [[entities/kubean.md|Kubean]]
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
