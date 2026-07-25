---
title: 容器运行时
description: 容器运行时（Container Runtime）是负责在节点上运行和管理容器的软件。它实现了 Kubernetes 的 CRI（Container
  Runtim...
summary: 容器运行时（Container Runtime）是负责在节点上运行和管理容器的软件。它实现了 Kubernetes 的 CRI（Container
  Runtim...
category: dictionary
tags:
- k8s
- glossary
- container-runtime
- containerd
- cri
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器运行时 是什么
- Container Runtime 详解
trigger_keywords:
- 容器运行时
- Container Runtime
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器运行时

> **英文名**: Container Runtime

## 概述

容器运行时（Container Runtime）是负责在节点上运行和管理容器的软件。它实现了 Kubernetes 的 CRI（Container Runtime Interface）接口，处理镜像拉取、容器创建和网络配置等操作。

## 核心概念/原理

### 主要运行时

- **containerd**：从 Docker 中拆分出的轻量级运行时，是当前 Kubernetes 的默认选择。
- **CRI-O**：专为 Kubernetes 设计的运行时，实现了最小化的 CRI 接口。
- **Docker Engine**：通过 dockershim（已移除）或 cri-dockerd 适配器支持 CRI。

### CRI 接口

Kubernetes 通过 CRI（Container Runtime Interface）与容器运行时通信。CRI 定义了一组 gRPC 接口：
- `RuntimeService`：管理容器生命周期。
- `ImageService`：管理容器镜像。

## 关键机制或特性

- 容器运行时分为高层级运行时（管理容器生命周期）和低层级运行时（如 runc，实际执行容器）。
- containerd 支持 OCI 标准镜像格式。
- CRI 使用 Unix domain socket 通信，默认路径 `/run/containerd/containerd.sock`。

## 使用场景与最佳实践

- 生产环境推荐使用 containerd 或 CRI-O。
- Docker 在 K8s v1.24 后不再直接支持，需使用 cri-dockerd 适配器。
- 配置镜像拉取超时和重试策略。
- 启用容器的 seccomp 和 AppArmor 安全配置。

## 参考链接

- [Container Runtime - Official Documentation](https://kubernetes.io/docs/setup/production-environment/container-runtimes/)

## Related

[[23-实体/03-运行时/containerd.md|containerd]] | [[23-实体/03-运行时/cri-o.md|CRI-O]]


<!-- risk-assessed -->
