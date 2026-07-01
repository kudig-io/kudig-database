---
title: containerd
description: containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过
  C...
summary: containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过
  C...
category: dictionary
tags:
- k8s
- glossary
- containerd
- cri
- container-runtime
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 是什么
- containerd 详解
trigger_keywords:
- containerd
- dictionary
prerequisites:
- kubectl-basics
---



# containerd

> **英文名**: containerd

## 概述

containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过 CRI 接口），负责容器的完整生命周期管理。

## 核心概念/原理

### 架构层次

```
kubelet → CRI → containerd → runc → Linux Kernel
                     ↓
              shim (per-container)
```

### 核心组件

| 组件 | 职责 |
|------|------|
| containerd daemon | 容器生命周期管理 |
| containerd-shim | 每个容器的独立进程，与 daemon 解耦 |
| runc | OCI 运行时规范实现 |
| ctr / crictl | 命令行工具 |

## 关键机制或特性

- **CRI 接口**：kubelet 通过 gRPC 调用 containerd 的 CRI 实现。
- **shim 架构**：containerd-shim 为每个容器独立运行，containerd 重启不影响容器。
- **镜像管理**：支持 OCI 和 Docker 镜像格式。
- **快照管理**：overlayfs 等快照驱动管理容器文件系统层。
- 配置文件位于 `/etc/containerd/config.toml`。

## 使用场景与最佳实践

- 使用 `crictl` 而非 `docker` 命令调试容器。
- 配置 mirror 加速镜像拉取（特别是国内环境）。
- 启用 `SystemdCgroup` 与 kubelet 保持一致。
- 监控 containerd 的 gRPC 延迟和容器启动时间指标。

## 参考链接

- [containerd Official](https://containerd.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/cri.md|CRI]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubelet.md|Kubelet]]
- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/container.md|Container]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/worker-node.md|Worker Node]]
