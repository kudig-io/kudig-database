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
last_updated: 2026-07
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

Hyperlight 是由 Microsoft 开发的轻量级虚拟机管理器（VMM），2024 年加入 CNCF Sandbox。它专为在毫秒级启动时间内运行函数式工作负载（Function Workloads）而设计。Hyperlight 创建超轻量的 micro-VM，每个 VM 可在 1-2 毫秒内启动，内存开销仅为 2-5 MB。Hyperlight 特别适合 Serverless、FaaS 和 AI Agent 安全沙箱场景，提供比容器更强的硬件级隔离，同时保持接近容器的启动速度和资源效率。

## 核心特性

- **极速启动**: 1-2ms VM 启动时间，接近进程创建速度
- **超低内存**: 每个 micro-VM 仅需 2-5MB 内存开销
- **硬件隔离**: 基于 Hypervisor（Microsoft Hypervisor / KVM）的硬件级沙箱
- **Host-Guest 通信**: 高效的 Host 函数调用和 Guest 回调机制
- **SandboxPool**: VM 实例池复用，减少创建开销
- **多语言 Guest**: 支持 Rust、Go、C、Python 编写的 Guest 代码

## 架构

Hyperlight 架构由 Host 和 Guest 两部分组成。Host 进程通过 Hyperlight SDK 创建 micro-VM——分配内存、加载 Guest 二进制文件到 Guest 内存空间、初始化 CPU 上下文。Guest 运行在硬件隔离的 VM 中，通过 Hypercall 与 Host 通信。Host 可以向 Guest 传递参数并调用 Guest 函数，Guest 也可以通过 Host Function 回调请求 Host 执行操作（如网络请求）。Guest 使用专用的内存布局和引导加载器，无需完整操作系统。支持 Microsoft Hypervisor（Windows/Azure）和 KVM（Linux）后端。

## Kubernetes 集成

Hyperlight 可作为 Kubernetes 中 AI Agent 和 Serverless 函数的安全沙箱运行时。通过自定义 RuntimeClass 或 Sidecar 模式集成。在 AI Agent 场景中，不可信的 Agent 代码运行在 Hyperlight micro-VM 中，通过 Host Function 受控地访问集群资源。与 containerd 的 shim 集成可实现将 Hyperlight VM 作为 Pod 的容器运行时替代。

## 生产使用场景

1. **AI Agent 沙箱**: 在隔离的 micro-VM 中运行不可信的 AI Agent 代码
2. **Serverless 函数**: 毫秒级启动的函数运行环境
3. **多租户隔离**: 在共享集群中为每个租户提供硬件级隔离
4. **安全计算**: 运行不可信代码（如用户提交的脚本）的沙箱

## 安装

```bash
# Rust SDK
cargo add hyperlight-host hyperlight-guest
# 示例: 创建 Sandbox 运行 Guest 函数
use hyperlight_host::sandbox::Sandbox;
let mut sandbox = Sandbox::new()?;
let result = sandbox.call_guest_function("add", &[1, 2])?;
# Kubernetes 集成
kubectl apply -f https://github.com/hyperlight-dev/hyperlight/deploy/kubernetes.yaml
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Hyperlight** | 极速启动、极低内存 | 较新、社区小 |
| Firecracker | AWS 生产验证、成熟 | 启动约 125ms、内存约 5MB |
| gVisor | 用户态内核、兼容性好 | 性能开销较大 |
| Kata Containers | 标准化、安全 | 启动较慢、资源开销大 |

## 架构定位

在 CNCF 生态中，Hyperlight 属于 **Runtime / Sandbox** 类别，代表了 micro-VM 在 AI Agent 和 Serverless 场景中的应用方向。它在隔离性与性能之间找到了新的平衡点。

## 参考链接

- [[实体/argocd.md|[[ArgoCD|argocd]]]]

## Related

- [[falco]] — Falco
- [[operator-framework]] — Operator Framework
- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hyperlight
- [[实体/urunc.md|[[urunc (Unikernel Container Runtime)|urunc]]]]
- [[实体/flatcar.md|Flatcar Container Linux]]
- [[实体/composefs.md|composefs]]
- [[实体/04-containerd-upgrade-migration.md|containerd 升级迁移]]
- [[实体/wasmedge.md|WasmEdge]]
- [[实体/spinkube.md|SpinKube]]
- [[实体/05-containerd-windows-support.md|containerd Windows 支持]]
- [[实体/02-containerd-v2-features.md|containerd 2.0 新特性]]
- [[实体/08-containerd-multi-tenant.md|containerd 多租户]]
- [[实体/k0s.md|K0s]]
- [[实体/03-containerd-security-hardening.md|containerd 安全加固]]
- [[实体/bootc.md|bootc]]
- [[实体/container2wasm.md|container2wasm]]
- [[实体/kubean.md|Kubean]]
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
