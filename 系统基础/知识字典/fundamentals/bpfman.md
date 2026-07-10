---
title: bpfman eBPF 管理器
description: bpfman 是 Red Hat 开源的 CNCF Sandbox 项目，作为系统级守护进程管理 eBPF 程序的加载和生命周期，解决多个应用争用
  eBPF 挂...
summary: bpfman 是 Red Hat 开源的 CNCF Sandbox 项目，作为系统级守护进程管理 eBPF 程序的加载和生命周期，解决多个应用争用
  eBPF 挂...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- ebpf
- daemon
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bpfman eBPF 管理器 是什么
- bpfman 详解
trigger_keywords:
- bpfman eBPF 管理器
- bpfman
- dictionary
prerequisites:
- kubernetes
---



# bpfman eBPF 管理器（bpfman）

## 概述

bpfman 是 Red Hat 开源的 CNCF Sandbox 项目，作为系统级守护进程管理 eBPF 程序的加载和生命周期，解决多个应用争用 eBPF 挂载点的冲突问题。

## 核心概念/原理

- **eBPF 管理器**：集中管理 eBPF 程序的加载和卸载
- **冲突解决**：多个程序挂载到同一 hook 点的优先级管理
- **CNCF Sandbox**：Red Hat 主导
- **系统服务**：以 systemd 服务方式运行

## 关键机制或特性

- gRPC API 管理 eBPF 程序
- 支持 XDP/TC/Tracepoint/Uprobe 等 hook 类型
- 优先级和顺序管理
- eBPF 映射（Map）的持久化
- Kubernetes CSI 驱动（K8s 集成）
- 与 Cilium/Tetragon 等兼容
- bpfilter 防火墙集成

## 使用场景与最佳实践

- 多个 eBPF 应用的共存管理
- eBPF 程序的生命周期管理
- K8s 节点上 eBPF 的统一部署
- 安全工具的 eBPF 程序管理
- eBPF 开发者的标准接口

## 参考链接

- https://bpfman.io/
- https://github.com/bpfman/bpfman

## Related

- [[系统基础/知识字典/networking/cilium.md|Cilium]]
- [[系统基础/知识字典/observability/pixie.md|Pixie]]
- [[系统基础/知识字典/security/falco.md|Falco]]
