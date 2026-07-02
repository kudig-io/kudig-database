---
title: Flatcar 容器操作系统
description: Flatcar Container Linux 是 Kinvolk（现微软）维护的不可变容器操作系统，是 CoreOS Container
  Linux 的社区分...
summary: Flatcar Container Linux 是 Kinvolk（现微软）维护的不可变容器操作系统，是 CoreOS Container Linux
  的社区分...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- os
- container
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flatcar 容器操作系统 是什么
- Flatcar 详解
trigger_keywords:
- Flatcar 容器操作系统
- Flatcar
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flatcar 容器操作系统（Flatcar）

## 概述

Flatcar Container Linux 是 Kinvolk（现微软）维护的不可变容器操作系统，是 CoreOS Container Linux 的社区分支，专为运行容器工作负载优化，提供自动更新和最小化攻击面。

## 核心概念/原理

- **不可变 OS**：只读根文件系统，通过原子更新交付
- **容器优化**：仅包含运行容器所需的最小系统组件
- **自动更新**：内置 update_engine 自动下载和应用更新
- **CoreOS 继承**：CoreOS Container Linux 的社区继任者

## 关键机制或特性

- Ignition 系统配置（替代 cloud-init）
- A/B 分区双系统（更新失败自动回滚）
- 最小化攻击面（无包管理器，无 SSH 密码登录）
- 自动安全更新
- 支持多种平台（AWS/Azure/GCP/Bare Metal/QEMU）
- 与 Fleet/Locksmith 协调更新策略

## 使用场景与最佳实践

- Kubernetes 节点的标准化操作系统
- 边缘/IoT 设备的不可变系统
- 安全合规要求下的最小化 OS
- 大规模集群的自动更新管理
- CoreOS 停服后的替代方案

## 参考链接

- https://www.flatcar.org/
- https://github.com/flatcar/Flatcar

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/bootc.md|bootc]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker.md|Docker]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd.md|containerd]]


<!-- risk-assessed -->
