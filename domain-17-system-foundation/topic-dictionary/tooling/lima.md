---
title: Lima Linux 虚拟机
description: 'Lima（Linux on Mac）是一个轻量级工具，在 macOS 上自动创建和管理 Linux 虚拟机，主要用于容器运行时（如 containerd/Doc...'
category: dictionary
tags:
- k8s
- glossary
- tooling
- development
- linux
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Lima Linux 虚拟机 是什么
- Lima 详解
trigger_keywords:
- Lima Linux 虚拟机
- Lima
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Lima Linux 虚拟机（Lima）

## 概述

Lima（Linux on Mac）是一个轻量级工具，在 macOS 上自动创建和管理 Linux 虚拟机，主要用于容器运行时（如 containerd/Docker）的开发和测试，是 colima 的底层引擎。

## 核心概念/原理

- **轻量 VM**：在 macOS 上快速启动 Linux 虚拟机
- **文件共享**：自动将宿主机目录共享到 VM（virtiofs）
- **端口转发**：自动将 VM 端口转发到宿主机
- **colima 底层**：colima（容器运行时管理器）基于 Lima 构建

## 关键机制或特性

- `limactl start` 创建 VM（支持多种 Linux 发行版模板）
- 内置 containerd/Docker/Podman 模板
- virtiofs 高性能文件共享
- 端口自动转发
- 支持多 VM 管理（`limactl list`）
- YAML 模板定义 VM 配置

## 使用场景与最佳实践

- macOS 上的 Linux 容器开发环境
- 容器运行时的本地测试
- CI/CD 中的 Linux 环境模拟
- Kubernetes 组件的本地开发
- colima 的底层引擎

## 参考链接

- https://lima-vm.io/
- https://github.com/lima-vm/lima

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/minikube.md|Minikube]]
- [[domain-17-system-foundation/topic-dictionary/tooling/k3s.md|K3s]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker.md|Docker]]
