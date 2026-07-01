---
title: About cgroup v2（关于 cgroup v2）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- About cgroup v2（关于 cgroup v2） 是什么
- 如何 About cgroup v2（关于 cgroup v2）
trigger_keywords:
- About
- cgroup
- v2
- 关于
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# About cgroup v2（关于 cgroup v2）

## 概述

在 Linux 上，控制组（control groups，简称 cgroups）用于限制分配给进程的资源。[[kubelet|kubelet]] 和底层容器运行时需要通过 cgroups 来强制执行 Pod 和容器的资源管理，包括 CPU/内存的请求（requests）和限制（limits）。Linux 上有两个版本的 cgroups：cgroup v1 和 cgroup v2。cgroup v2 是新一代的 cgroup API。

## 核心概念/原理

- **cgroup v2**：Linux cgroup API 的下一个版本，提供了一个统一的控制系统，具有增强的资源管理能力。
- **与 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 的集成**：自 v1.25 起，cgroup v2 在 Kubernetes 中达到 Stable 状态。kubelet 能够自动检测操作系统是否运行在 cgroup v2 上，并自动适配，无需额外配置。
- **Kubernetes v1.35 弃用 cgroup v1**：cgroup v1 已被弃用，kubelet 默认不再在 cgroup v1 节点上启动。如需禁用该检查，集群管理员需在 kubelet 配置文件中设置 `failCgroupV1: false`。

## 关键机制或特性

cgroup v2 相比 cgroup v1 提供了多项改进：

- **统一层次结构（Unified Hierarchy）**：将所有控制器挂载到单一层次结构中，简化资源管理
- **增强的资源管理**：更细粒度的资源控制和更好的隔离能力
- **内存 QoS（MemoryQoS）**：某些 Kubernetes 特性（如 MemoryQoS）专门依赖 cgroup v2 原语来提升内存的服务质量

### 系统要求

- Linux 内核版本需支持 cgroup v2（推荐发行版默认启用）
- 容器运行时需支持 cgroup v2

### 检查 cgroup 版本

在节点上运行以下命令：

```bash
stat -fc %T /sys/fs/cgroup/
```

- 输出为 `cgroup2fs` 表示使用 cgroup v2
- 输出为 `tmpfs` 表示使用 cgroup v1

## 使用场景

- 需要更细粒度资源控制和更好隔离能力的生产环境
- 使用 MemoryQoS 等仅支持 cgroup v2 的 Kubernetes 特性
- 新部署的集群应优先使用 cgroup v2，以获得长期支持和更好性能

## 最佳实践/注意事项

- **推荐使用默认启用 cgroup v2 的 Linux 发行版**，而非手动修改内核启动参数
- 若必须手动启用，可在 GRUB 配置中添加 `systemd.unified_cgroup_hierarchy=1`，然后执行 `update-grub`
- 迁移到 cgroup v2 前，确保满足内核和运行时要求
- 如果有应用直接访问 cgroup 文件系统，需要将其更新到支持 cgroup v2 的版本。例如：某些监控代理、安全工具或自定义脚本
- 用户直接访问 cgroup 文件系统时，会注意到 v1 和 v2 的 API 差异

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/cgroups/

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/annotations.md|注解]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/bpfman.md|bpfman eBPF 管理器]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cloud-controller-manager.md|Cloud Controller Manager（云控制器管理器）]]
