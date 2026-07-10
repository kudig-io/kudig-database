---
title: Kairos 不可变 OS
description: Kairos（原 c3os）是 Spectro Cloud 开源的 CNCF Sandbox 项目，将任意 Linux 发行版转换为不可变的容器操作系统，支持用...
summary: Kairos（原 c3os）是 Spectro Cloud 开源的 CNCF Sandbox 项目，将任意 Linux 发行版转换为不可变的容器操作系统，支持用...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- os
- edge
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kairos 不可变 OS 是什么
- Kairos 详解
trigger_keywords:
- Kairos 不可变 OS
- Kairos
- dictionary
prerequisites:
- kubernetes
---



# Kairos 不可变 OS（Kairos）

## 概述

Kairos（原 c3os）是 Spectro Cloud 开源的 CNCF Sandbox 项目，将任意 Linux 发行版转换为不可变的容器操作系统，支持用容器镜像管理整个 OS，适用于边缘和 Kubernetes 节点。

## 核心概念/原理

- **容器即 OS**：用 OCI 镜像定义完整的操作系统
- **任意发行版**：基于 Alpine/Ubuntu/openSUSE 等构建
- **CNCF Sandbox**：Spectro Cloud 主导
- **边缘优化**：专为边缘设备设计

## 关键机制或特性

- A/B 分区原子升级和回滚
- cloud-init 系统配置
- P2P 网络（节点自发现和自组网）
- K3s/K0s 内置集成
- UKI（Unified Kernel Image）支持
- Elemental Operator K8s 管理
- 安全启动（Secure Boot）

## 使用场景与最佳实践

- 边缘设备的 OS 管理
- K8s 节点的标准化 OS
- 不可变基础设施的 OS 层
- IoT 设备的远程 OS 更新
- 多发行版的统一 OS 管理

## 参考链接

- https://kairos.io/
- https://github.com/kairos-io/kairos

## Related

- [[domain-17-system-foundation/知识字典/fundamentals/flatcar.md|Flatcar]]
- [[domain-17-system-foundation/知识字典/tooling/bootc.md|bootc]]
- [[domain-17-system-foundation/知识字典/tooling/k3s.md|K3s]]
