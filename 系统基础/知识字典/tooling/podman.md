---
title: Podman
description: Podman 是 Red Hat 开发的无守护进程（daemonless）容器引擎，兼容 Docker CLI 但无需 Docker daemon。它支持
  ro...
summary: Podman 是 Red Hat 开发的无守护进程（daemonless）容器引擎，兼容 Docker CLI 但无需 Docker daemon。它支持
  ro...
category: dictionary
tags:
- k8s
- glossary
- podman
- container
- docker-alternative
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Podman 是什么
- Podman 详解
trigger_keywords:
- Podman
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Podman

> **英文名**: Podman

## 概述

Podman 是 Red Hat 开发的无守护进程（daemonless）容器引擎，兼容 Docker CLI 但无需 Docker daemon。它支持 rootless 运行容器，是 Linux 系统上 Docker 的安全替代方案。

## 核心概念/原理

### 与 Docker 对比

| 特性 | Podman | Docker |
|------|--------|--------|
| 架构 | 无 daemon（fork-exec） | Client-Daemon |
| Rootless | 原生支持 | 需要额外配置 |
| Pod 概念 | 原生（类似 K8s Pod） | 无 |
| Docker API | 兼容大部分命令 | 原生 |
| Compose | podman-compose | docker-compose |

### Pod 概念

Podman 原生支持 Pod（一组共享网络和存储的容器），类似 Kubernetes Pod。

## 关键机制或特性

- **podman generate kube**：将容器/Pod 转换为 K8s YAML。
- **podman play kube**：运行 K8s YAML（本地调试）。
- **Quadlet**：systemd 集成管理容器。
- **Podman Desktop**：跨平台 GUI 管理容器。
- 支持 Docker 镜像格式和 OCI 镜像格式。

## 使用场景与最佳实践

- Linux 服务器使用 Podman 替代 Docker 提升安全性（rootless）。
- 使用 `podman generate kube` 快速将容器配置转为 K8s YAML。
- 使用 `podman play kube` 本地测试 K8s 配置。
- 配合 systemd Quadlet 管理生产容器。
- 注意 Podman 与 Docker 的细微差异（网络、卷挂载等）。

## 参考链接

- [Podman Official](https://podman.io/)

## Related

- [[系统基础/topic-dictionary/fundamentals/docker.md|Docker]]
- [[系统基础/topic-dictionary/fundamentals/containerd.md|Containerd]]
- [[系统基础/topic-dictionary/fundamentals/container.md|Container]]
- [[系统基础/topic-dictionary/workloads/pod.md|Pod]]
- [[系统基础/topic-dictionary/security/security-context.md|Security Context]]


<!-- risk-assessed -->
