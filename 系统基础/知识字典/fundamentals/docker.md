---
title: Docker
description: Docker 是最广泛使用的容器平台，包含 Docker Engine（运行时）、Docker CLI 和 Docker Buildx（构建工具）。虽然
  Kub...
summary: Docker 是最广泛使用的容器平台，包含 Docker Engine（运行时）、Docker CLI 和 Docker Buildx（构建工具）。虽然
  Kub...
category: dictionary
tags:
- k8s
- glossary
- docker
- container
- oci
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Docker 是什么
- Docker 详解
trigger_keywords:
- Docker
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Docker

> **英文名**: Docker

## 概述

Docker 是最广泛使用的容器平台，包含 Docker Engine（运行时）、Docker CLI 和 Docker Buildx（构建工具）。虽然 Kubernetes 已移除对 Docker 的直接支持（dockershim 弃用），但 Docker 镜像格式仍是 OCI 标准的基础。

## 核心概念/原理

### Docker 与 K8s 的关系

| 组件 | K8s 中使用 |
|------|------------|
| Docker Engine | 已被 containerd/CRI-O 替代 |
| Docker Image | 仍然使用（OCI 兼容） |
| Docker CLI | 开发环境仍广泛使用 |
| Docker Compose | 本地多容器开发 |
| Docker Buildx | 构建多架构镜像 |

### OCI 标准

Docker 推动了容器技术的发展，其镜像格式和运行时规范已被 OCI（Open Container Initiative）标准化。

## 关键机制或特性

- **Docker Desktop**：macOS/Windows 上的开发环境。
- **Docker Buildx**：多架构镜像构建（amd64/arm64）。
- **Docker Compose**：本地多容器编排。
- **docker save/load**：镜像离线传输。
- **BuildKit**：Docker 内置的高级构建引擎。

## 使用场景与最佳实践

- 开发环境继续使用 Docker Desktop/Docker CLI。
- 生产 K8s 集群使用 containerd 或 CRI-O 作为运行时。
- 使用 `docker buildx build --platform linux/amd64,linux/arm64` 构建多架构镜像。
- 使用 `.dockerignore` 减少构建上下文大小。
- CI/CD 中使用 Docker-in-Docker 或 Kaniko 构建镜像。

## 参考链接

- [Docker Official](https://docs.docker.com/)

## Related

- [[系统基础/知识字典/fundamentals/containerd.md|Containerd]]
- [[系统基础/知识字典/fundamentals/cri.md|CRI]]
- [[系统基础/知识字典/fundamentals/container.md|Container]]
- [[系统基础/知识字典/workloads/pod.md|Pod]]
- [[系统基础/知识字典/fundamentals/cri-o.md|CRI-O]]


<!-- risk-assessed -->
