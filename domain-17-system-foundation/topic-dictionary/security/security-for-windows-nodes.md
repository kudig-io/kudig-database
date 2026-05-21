---
title: Windows 节点安全
description: '# Windows 节点安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Windows 节点安全 是什么
- 如何 Windows 节点安全
trigger_keywords:
- Windows
- 节点安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

# Windows 节点安全

## 概述

本页面描述了针对 Windows 操作系统的安全考虑和最佳实践。Windows 节点在 [[entities/kubernetes|kubernetes]] 集群中的行为与 Linux 节点存在显著差异，特别是在 Secret 保护、容器用户和 Pod 安全隔离方面。

## 核心概念/原理

### Secret 数据保护

在 Windows 节点上，Secret 数据会以**明文**形式写入节点的本地存储（而 Linux 使用 tmpfs/内存文件系统）。因此，作为集群管理员，需要采取额外的保护措施。

### 容器用户

- Windows 容器提供两个默认用户账户：**ContainerUser** 和 **ContainerAdministrator**。
- 可以使用 `RunAsUsername` 为 Windows Pod 或容器指定以特定用户身份执行容器进程，这大致相当于 Linux 的 `RunAsUser`。
- 也可以在容器构建过程中向镜像添加本地用户，或者利用 **Group Managed Service Accounts（gMSA）** 以 Active Directory 身份运行容器。

### Pod 级安全隔离

- Linux 特有的 Pod 安全上下文机制（如 SELinux、AppArmor、Seccomp 和自定义 POSIX capabilities）在 Windows 节点上**不受支持**。
- Windows 不支持特权容器（Privileged containers），但可以使用 **HostProcess 容器**来执行许多在 Linux 上由特权容器完成的任务。

## 关键机制或特性

- **文件 ACL（访问控制列表）**：用于保护 Windows 节点上 Secrets 的文件位置。
- **BitLocker**：提供卷级加密，保护节点本地存储上的数据。
- **HostProcess 容器**：允许在 Windows 主机上以特权方式运行进程，是 Windows 上特权操作的替代方案。
- **RunAsUsername / gMSA**：支持以特定用户或 Active Directory 身份运行容器。

## 使用场景

- 在 Windows 节点上运行需要访问 Secret 的容器化工作负载。
- 需要在 Windows 容器中执行特权管理任务（如网络配置、系统管理）。
- 使用 Active Directory 身份集成运行 Windows 容器的企业环境。

## 最佳实践/注意事项

- **使用文件 ACL 和 BitLocker**：由于 Windows 不使用 tmpfs，必须额外保护 Secret 文件位置和存储卷。
- **理解 ContainerUser 与 ContainerAdministrator 的区别**：根据应用需求选择最小权限的默认账户，避免不必要的管理员权限。
- **使用 HostProcess 容器替代特权容器**：在 Windows 上执行需要主机访问权限的任务时，优先使用 HostProcess 容器。
- **注意 Linux 安全机制在 Windows 上无效**：不要期望 SELinux、AppArmor、Seccomp 等在 Windows 节点上生效；需要采用 Windows 原生的安全控制措施。
- **为本地存储启用 BitLocker 加密**：降低物理访问或底层存储泄露导致的数据暴露风险。

## 参考链接

- https://kubernetes.io/docs/concepts/security/windows-security/
