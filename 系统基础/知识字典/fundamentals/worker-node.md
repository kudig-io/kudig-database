---
title: 工作节点
description: Worker Node（工作节点）是 Kubernetes 集群中运行用户工作负载的机器。它是集群中实际执行 Pod 的节点，通常数量远多于控制平面节点。...
summary: Worker Node（工作节点）是 Kubernetes 集群中运行用户工作负载的机器。它是集群中实际执行 Pod 的节点，通常数量远多于控制平面节点。...
category: dictionary
tags:
- k8s
- glossary
- node
- worker
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 工作节点 是什么
- Worker Node 详解
trigger_keywords:
- 工作节点
- Worker Node
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工作节点

> **英文名**: Worker Node

## 概述

Worker Node（工作节点）是 Kubernetes 集群中运行用户工作负载的机器。它是集群中实际执行 Pod 的节点，通常数量远多于控制平面节点。

## 核心概念/原理

### 核心特性
- 运行 kubelet、kube-proxy 和容器运行时。
- 通过标签标识角色（如 `node-role.kubernetes.io/worker`）。
- 由控制平面管理，但本身不运行控制平面组件。

### 与控制平面节点的对比

| 特性 | Worker Node | Control Plane Node |
|------|------------|-------------------|
| 运行组件 | kubelet, kube-proxy, 容器 | API Server, etcd, Scheduler |
| 主要职责 | 运行用户工作负载 | 管理集群状态 |
| 数量 | 可水平扩展（数十至数千） | 通常 3-5 个（高可用） |
| 污点 | 通常无 | 有控制平面污点 |

## 关键机制或特性

- Worker Node 通过 kubelet 向 API Server 注册自身。
- 节点容量（Capacity）和可分配资源（Allocatable）决定了可运行的 Pod 数量。
- `--max-pods` 参数限制单节点最大 Pod 数（默认 110）。

## 使用场景与最佳实践

- 根据工作负载类型对 Worker Node 进行分类（如 GPU 节点、高内存节点）。
- 使用节点池（Node Pool）管理不同规格的 Worker Node。
- 监控 Worker Node 的资源利用率和 Pod 密度。
- 配置节点自动扩缩容（Cluster Autoscaler / Karpenter）应对负载波动。

## 参考链接

- [Worker Node - Official Documentation](https://kubernetes.io/docs/concepts/architecture/nodes/)

## Related

- [[系统基础/知识字典/workloads/pod.md|Pod]]
- [[系统基础/知识字典/fundamentals/container.md|Container]]
- [[系统基础/知识字典/fundamentals/namespace.md|Namespace]]
- [[系统基础/知识字典/fundamentals/cluster.md|Cluster]]
- [[系统基础/知识字典/fundamentals/control-plane.md|Control Plane]]


<!-- risk-assessed -->
