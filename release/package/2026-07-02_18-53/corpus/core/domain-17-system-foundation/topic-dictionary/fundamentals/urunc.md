---
title: urunc 微库运行时
description: urunc 是 Nubificus 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Unikernel 和轻量虚拟机，利用
  Unike...
summary: urunc 是 Nubificus 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Unikernel 和轻量虚拟机，利用
  Unike...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- container-runtime
- unikernel
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- urunc 微库运行时 是什么
- urunc 详解
trigger_keywords:
- urunc 微库运行时
- urunc
- dictionary
prerequisites:
- kubernetes
---



# urunc 微库运行时（urunc）

## 概述

urunc 是 Nubificus 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Unikernel 和轻量虚拟机，利用 Unikernel 的极小攻击面和快速启动特性，为安全敏感工作负载提供超轻量隔离方案。

## 核心概念/原理

- **Unikernel 支持**：在 K8s 上运行 Unikernel（Unikraft/MirageOS/OSv）
- **超轻量隔离**：每个容器运行在独立的微型内核中
- **快速启动**：毫秒级启动时间
- **CNCF Sandbox**：Nubificus 主导

## 关键机制或特性

- 支持 Unikraft、MirageOS、OSv 等 Unikernel
- 基于 Firecracker/QEMU 的轻量 VM 隔离
- OCI 兼容的镜像格式
- 与 containerd shim 集成
- 极低内存开销（MB 级）
- Rum 命令行工具

## 使用场景与最佳实践

- 安全敏感工作负载的强隔离
- Serverless 函数的快速启动容器
- 边缘设备的超轻量运行时
- 零信任架构中的工作负载隔离
- 替代 gVisor/Kata 的轻量方案

## 参考链接

- https://urunc.io/
- https://github.com/nubificus/urunc

## Related

- [[domain-17-system-foundation/知识字典/fundamentals/kata-containers.md|Kata Containers]]
- [[domain-17-system-foundation/知识字典/fundamentals/runc.md|runc]]
- [[domain-17-system-foundation/知识字典/fundamentals/kuasar.md|Kuasar]]
