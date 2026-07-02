---
title: runc
description: runc 是 OCI（Open Container Initiative）标准的容器运行时参考实现，负责将 OCI 镜像和配置转换为 Linux
  容器进程。它是...
summary: runc 是 OCI（Open Container Initiative）标准的容器运行时参考实现，负责将 OCI 镜像和配置转换为 Linux
  容器进程。它是...
category: dictionary
tags:
- k8s
- glossary
- runc
- oci
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
- runc 是什么
- runc 详解
trigger_keywords:
- runc
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# runc

> **英文名**: runc

## 概述

runc 是 OCI（Open Container Initiative）标准的容器运行时参考实现，负责将 OCI 镜像和配置转换为 Linux 容器进程。它是 containerd 和 CRI-O 底层的实际容器创建引擎。

## 核心概念/原理

### 运行时层次

```
kubelet → CRI → containerd/CRI-O → runc → Linux Kernel (namespaces/cgroups)
```

### OCI 运行时规范

- 定义了容器进程的创建、启动、停止、删除接口。
- runc 使用 Linux namespaces（pid、net、mnt、ipc、uts、user）实现隔离。
- 使用 cgroups 实现资源限制。
- 使用 capabilities 和 seccomp 实现安全沙箱。

## 关键机制或特性

- **轻量级**：runc 仅创建容器，不管理生命周期（由上层 daemon 管理）。
- **OCI 合规**：完全遵循 OCI Image 和 Runtime 规范。
- **seccomp 支持**：限制容器可用的系统调用。
- **rootless 模式**：无 root 权限运行容器。
- 配置文件：`config.json`（OCI bundle 格式）。

## 使用场景与最佳实践

- 通常不需要直接操作 runc，通过 containerd/CRI-O 间接使用。
- 追求更安全的沙箱可考虑 crun（C 实现，更快）或 gVisor。
- 调试时可使用 `runc exec` 进入容器。
- 关注 runc 的 CVE 更新（如 CVE-2024-21626）。
- rootless 容器适合开发环境的安全隔离。

## 参考链接

- [runc GitHub](https://github.com/opencontainers/runc)

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd.md|Containerd]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cri-o.md|CRI-O]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cri.md|CRI]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/container.md|Container]]
- [[domain-17-system-foundation/topic-dictionary/security/security-context.md|Security Context]]


<!-- risk-assessed -->
