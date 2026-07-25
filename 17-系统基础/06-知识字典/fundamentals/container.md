---
title: 容器
description: 容器是一种轻量级的操作系统级虚拟化技术，通过 Linux 内核的 namespace 和 cgroup 实现进程隔离和资源限制，是 Docker/Contain...
summary: 容器是一种轻量级的操作系统级虚拟化技术，通过 Linux 内核的 namespace 和 cgroup 实现进程隔离和资源限制，是 Docker/Contain...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- runtime
- oci
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器 是什么
- Container 详解
trigger_keywords:
- 容器
- Container
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器（Container）

## 概述

容器是一种轻量级的操作系统级虚拟化技术，通过 Linux 内核的 namespace 和 cgroup 实现进程隔离和资源限制，是 Docker/Containerd 等运行时的核心构建单元。

## 核心概念/原理

- **Linux Namespace**：PID/Network/Mount/UTS/IPC/User 六种隔离维度
- **Cgroup**：CPU/Memory/IO/PID 等资源限制
- **OCI 标准**：运行时规范和镜像规范
- **UnionFS**：分层文件系统实现

## 关键机制或特性

- 容器 = 进程 + namespace + cgroup + rootfs
- 镜像是只读层，容器是读写层
- 容器运行时（runc）负责创建和管理
- 容器间共享内核，隔离通过 namespace 实现
- 生命周期：create → start → running → stop → remove
- 资源限制通过 cgroup v1/v2 配置
- 健康检查通过进程探针实现

## 使用场景与最佳实践

- 应用容器化（微服务部署）
- CI/CD 构建环境隔离
- 多租户安全隔离
- 资源配额和限流
- 不可变基础设施
- 最佳实践：单进程、非 root、只读 rootfs、健康检查

## 参考链接

- https://kubernetes.io/docs/concepts/containers/
- https://opencontainers.org/

## Related

- [[17-系统基础/06-知识字典/fundamentals/docker.md|Docker]]
- [[17-系统基础/06-知识字典/fundamentals/runc.md|runc]]
- [[17-系统基础/06-知识字典/fundamentals/containerd.md|containerd]]


<!-- risk-assessed -->
