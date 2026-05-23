---
title: 针对 Pod 和容器的 Linux 内核安全约束
description: '# 针对 Pod 和容器的 Linux 内核安全约束'
category: dictionary
tags:
- k8s
- glossary
- terminology
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 针对 Pod 和容器的 Linux 内核安全约束 是什么
- 如何 针对 Pod 和容器的 Linux 内核安全约束
trigger_keywords:
- 针对
- Pod
- 和容器的
- Linux
- 内核安全约束
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# 针对 Pod 和容器的 Linux 内核安全约束

## 概述

本页面概述了可用于加固 Pod 和容器的 Linux 内核安全模块和约束。这些功能是 Linux 内核内置的安全能力，常见特性包括 seccomp、AppArmor 和 SELinux。在 Pod 规范中使用 `securityContext` 字段来配置这些约束。

## 核心概念/原理

Linux 内核提供了多种安全特性来改善隔离并加固容器化工作负载。[[entities/kubernetes|[[Kubernetes|kubernetes]]]] 允许通过 Pod 的 `securityContext` 配置这些特性，还支持其他安全设置，如特定的 Linux capabilities 或使用 UID/GID 的文件访问权限。

在配置这些内核级安全能力之前，建议首先考虑实施**网络级隔离**，并尽量以**非 root 用户**运行工作负载。如果确实需要 root 权限，可以考虑使用用户命名空间（Userer Namespaces|User Namespaces]]espaces]]）来减少主机上的权限。

## 关键机制或特性

### seccomp（安全计算模式）

seccomp 用于过滤进程可以发起的系统调用（syscalls）。容器运行时为每个容器定义了一个默认的 seccomp 配置文件。Kubernetes 允许自动将节点上加载的 seccomp 配置文件应用到 Pod 和容器。

- **配置方式**：在 Pod 或容器规范的 `securityContext.seccompProfile` 中指定配置文件类型（`RuntimeDefault`、`Localhost`）或自定义配置文件路径。
- **注意事项**：seccomp 是低级别的安全配置，除非需要细粒度控制，否则建议使用容器运行时附带的**默认 seccomp 配置文件**。
- **特权容器覆盖**：特权容器会以 `Unconfined` seccomp 配置文件运行，覆盖任何在清单中指定的 seccomp 配置文件。

### AppArmor

AppArmor 是一个 Linux 内核安全模块，通过配置文件将程序限制在有限的资源集合内。每个配置文件可以运行在**强制执行模式**（阻止对不允许资源的访问）或**投诉模式**（仅报告违规）。

- **配置方式**：在容器规范中通过注解指定加载的 AppArmor 配置文件。
- **用途**：限制容器允许执行的操作，并通过系统日志提供更好的审计能力。
- **特权容器覆盖**：特权容器会忽略任何已应用的 AppArmor 配置文件。

### SELinux

SELinux 是一个 Linux 内核安全模块，通过为对象分配安全标签来实施访问控制策略。当具有 SELinux 标签的进程尝试访问文件时，SELinux 服务器会检查该进程的安全策略是否允许访问。

- **配置方式**：在 Pod 规范的 `securityContext.seLinuxOptions` 中设置标签。
- **用途**：限制容器访问其自身文件系统之外的文件、应用程序、端口和进程。
- **特权容器覆盖**：特权容器会以 `unconfined_t` SELinux 域运行。

### AppArmor 与 SELinux 的区别

| 特性 | AppArmor | SELinux |
|------|----------|---------|
| 配置方式 | 使用配置文件定义资源访问 | 使用应用于特定标签的策略 |
| 资源识别 | 使用文件路径 | 使用资源的索引节点（inode） |

通常，Linux 节点操作系统默认包含 AppArmor 或 SELinux 其中之一。

### 特权容器（Privileged Containers）

特权容器显式覆盖或撤销许多其他加固设置。当容器的 `securityContext.privileged` 设置为 `true` 时：

- seccomp 变为 `Unconfined`
- AppArmor 配置文件被忽略
- SELinux 变为 `unconfined_t`
- 获得所有 Linux capabilities，包括 `CAP_SYS_ADMIN`、`CAP_NET_ADMIN` 等

**大多数场景应避免使用特权容器**，而应通过 `securityContext.capabilities` 授予容器所需的特定 capabilities。

## 使用场景

- 需要对容器进行细粒度系统调用控制的场景（seccomp）。
- 希望限制程序对文件系统、网络和能力的访问（AppArmor）。
- 需要基于标签实施严格的强制访问控制策略（SELinux）。
- 运行不可信代码或高敏感性工作负载，需要额外的内核级隔离。

## 最佳实践/注意事项

- **优先以非 root 运行**：在 Pod 清单中设置 `runAsNonRoot: true` 和具体的 `runAsUser`/`runAsGroup`。
- **避免特权容器**：除非必须执行操作系统管理任务（如操作网络栈或访问硬件设备），否则不要使用 `privileged: true`。
- **使用默认 seccomp 配置文件**：如需更强隔离，考虑使用沙箱（如 gVisor），而非自行管理大量自定义 seccomp 配置文件。
- **管理自定义配置**：在大规模环境中管理 seccomp、AppArmor 和 SELinux 的自定义配置文件可能具有挑战性，可使用 **Kubernetes Security Profiles Operator** 等工具进行管理。
- **用户命名空间**：若容器内需要 root 权限，可设置 `hostUsers: false` 在用户命名空间中运行容器，使其在主机上以非 root 身份运行（该功能仍在持续发展中）。

## 参考链接

- https://kubernetes.io/docs/concepts/security/linux-kernel-security-constraints/
