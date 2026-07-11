---
title: youki [entities]
description: '## 概述'
summary: 'youki 是一个用 Rust 实现的 OCI 容器运行时，作为 runc 的替代品。'
category: entities
tags:
- k8s
- cncf
- runtime
- youki
- containerd
- cri-o
- crd
- operator
- wasm
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- youki 是什么
- 如何 youki
trigger_keywords:
- youki
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# youki

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

youki 是一个用 Rust 实现的 OCI 容器运行时（OCI Runtime），作为 runc 的替代品，2022 年加入 CNCF 沙箱。它完全兼容 OCI Runtime Specification，同时利用 Rust 语言的内存安全特性减少潜在的安全漏洞（如 buffer overflow、use-after-free 等 C 语言常见问题）。youki 可与 containerd、CRI-O、Podman 等高级容器运行时集成，作为底层容器执行引擎。youki 还实验性支持 Wasm 运行时特性，可以在同一运行时中运行传统 Linux 容器和 WebAssembly 模块。作为 Rust 实现的运行时，youki 还具有优秀的并发性能和更低的资源开销。

## 核心能力

- **OCI 兼容**: 完全兼容 OCI Runtime Specification，可作为 runc 直接替代
- **内存安全**: 利用 Rust 的所有权系统，消除 buffer overflow、data race 等内存安全漏洞
- **高性能**: Rust 的零成本抽象和优秀的并发模型，性能与 runc 相当或更优
- **Rootless 模式**: 支持非特权用户运行容器（rootless containers）
- **Wasm 支持**: 实验性支持通过 Wasm 运行时运行 WebAssembly 模块
- **cgroups v2**: 完整支持 cgroups v2 资源管理

## 架构

youki 作为底层 OCI Runtime，遵循 OCI 规范设计：

- **youki 二进制**: 替代 runc 的容器运行时二进制，实现 OCI Runtime CLI 接口
- **libcontainer**: youki 的核心库，管理容器生命周期（create/start/kill/delete）
- **Namespaces**: 利用 Linux namespace 实现容器隔离（pid/net/mnt/uts/ipc/user）
- **Cgroups**: 通过 cgroups v1/v2 管理资源限制（CPU/内存/IO/PID）
- **Linux Capabilities**: 精细化的 Linux capability 权限控制
- **Seccomp**: 系统调用过滤，限制容器可用的 syscall

容器生命周期：`containerd/CRI-O → youki create → youki start → youki kill → youki delete`

## K8s 集成

youki 作为 OCI Runtime 与 Kubernetes 集成。在节点上配置 containerd 或 CRI-O 使用 youki 替代默认的 runc（在 containerd config.toml 中设置 `runtime = "youki"`）。youki 处理容器的创建、启动、停止和删除操作，由上层 CRI（containerd/CRI-O）调用。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的容器运行时接口（CRI）完全兼容——替换 youki 不需要修改任何 Pod 或 Deployment 配置。

## 生产场景

1. **安全敏感环境**: 利用 Rust 内存安全减少容器逃逸漏洞风险
2. **Rootless 容器**: 在无 root 权限环境中运行安全容器
3. **边缘轻量运行时**: 边缘设备上使用 Rust 运行时获得更好的资源效率
4. **Wasm + 容器混合**: 在同一节点上运行传统容器和 Wasm 模块

## 安装

```bash
# 从源码安装 youki
git clone https://github.com/containers/youki.git
cd youki && make youki
sudo mv youki /usr/local/bin/

# 验证安装
youki --version
youki info

# 运行容器
youki create -b /tmp/container-bundle my-container
youki start my-container
youki delete my-container

# 配置 containerd 使用 youki
# 编辑 /etc/containerd/config.toml:
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.youki]
#   runtime_type = "io.containerd.runc.v2"
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.youki.options]
#   BinaryName = "youki"
```

## 对比

| 特性 | youki | runc | crun | runsc (gVisor) |
|------|-------|------|------|----------------|
| 语言 | Rust | Go | C | Go |
| 内存安全 | ✅ | ⚠️ | ❌ | ⚠️ |
| 性能 | 高 | 高 | 高 | 中（开销） |
| OCI 兼容 | ✅ | ✅ | ✅ | ✅ |

## 架构定位

在 CNCF 生态中，youki 属于 **Runtime** 类别，为云原生应用提供内存安全的容器运行时能力。

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[kairos]] — Kairos
- [[kaito]] — KAITO
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- youki
- [[概念/container-runtime-comparison.md|[[Container Runtime|Container Runtime]]me Comparison|Container Runtime Comparison]]]] — Cross-reference
- [[概念/docker-architecture.md|[[Docker Architecture and Container Runtime|Docker Architecture and Container Runtime]]]] — Cross-reference
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
