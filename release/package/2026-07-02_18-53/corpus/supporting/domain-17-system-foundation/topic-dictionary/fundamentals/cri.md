---
title: 容器运行时接口
description: CRI（Container Runtime Interface，容器运行时接口）是 Kubernetes 定义的一组 gRPC 接口标准，用于
  kubelet ...
summary: CRI（Container Runtime Interface，容器运行时接口）是 Kubernetes 定义的一组 gRPC 接口标准，用于 kubelet
  ...
category: dictionary
tags:
- k8s
- glossary
- cri
- container-runtime
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器运行时接口 是什么
- CRI (Container Runtime Interface) 详解
trigger_keywords:
- 容器运行时接口
- CRI (Container Runtime Interface)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器运行时接口

> **英文名**: CRI (Container Runtime Interface)

## 概述

CRI（Container Runtime Interface，容器运行时接口）是 Kubernetes 定义的一组 gRPC 接口标准，用于 kubelet 与容器运行时之间的通信。CRI 使 Kubernetes 能够支持多种容器运行时实现。

## 核心概念/原理

### CRI 接口组成

| 服务 | 职责 |
|------|------|
| **RuntimeService** | 管理容器生命周期（创建、启动、停止、删除） |
| **ImageService** | 管理容器镜像（拉取、检查、删除） |

### 通信方式

```
kubelet ──gRPC──> /run/containerd/containerd.sock ──> containerd
           │
           └──> /run/crio/crio.sock ──> CRI-O
```

### 支持的运行时

- **containerd**：K8s 默认运行时。
- **CRI-O**：专为 Kubernetes 设计的最小化运行时。
- **cri-dockerd**：Docker 的 CRI 适配器（Docker 已从 K8s 1.24 起不再直接支持）。

## 关键机制或特性

- CRI 使用 Unix domain socket 通信。
- CRI 版本与 Kubernetes 版本有兼容性要求。
- `crictl` 是 CRI 兼容运行时的调试命令行工具。
- CRI 从 K8s v1.24 起成为唯一的运行时集成方式（移除了 dockershim）。

## 使用场景与最佳实践

- 生产环境选择 containerd 或 CRI-O。
- 使用 `crictl` 调试容器和镜像。
- 确保运行时版本与 Kubernetes 版本兼容。
- 监控运行时的操作延迟和错误率。

## 参考链接

- [CRI (Container Runtime Interface) - Official Documentation](https://kubernetes.io/docs/concepts/architecture/cri/)

## Related

- [[domain-17-system-foundation/知识字典/workloads/pod.md|Pod]]
- [[domain-17-system-foundation/知识字典/fundamentals/container.md|Container]]
- [[domain-17-system-foundation/知识字典/fundamentals/node.md|Node]]
- [[domain-17-system-foundation/知识字典/fundamentals/namespace.md|Namespace]]
- [[domain-17-system-foundation/知识字典/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
