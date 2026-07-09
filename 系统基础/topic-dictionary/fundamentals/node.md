---
title: 节点
description: Node（节点）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机。节点上运行 kubelet 和容器运行时，负责执行用户的工作负载（Pod）。...
summary: Node（节点）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机。节点上运行 kubelet 和容器运行时，负责执行用户的工作负载（Pod）。...
category: dictionary
tags:
- k8s
- glossary
- node
- kubelet
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点 是什么
- Node 详解
trigger_keywords:
- 节点
- Node
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点

> **英文名**: Node

## 概述

Node（节点）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机。节点上运行 kubelet 和容器运行时，负责执行用户的工作负载（Pod）。

## 核心概念/原理

### 节点组件

每个节点运行以下核心组件：

- **kubelet**：节点代理，接收来自 API Server 的 Pod 规格，确保 Pod 中的容器正常运行。
- **kube-proxy**：网络代理，维护节点上的网络规则，实现 Service 的负载均衡。
- **Container Runtime**：容器运行时（如 containerd），负责拉取镜像和运行容器。

### 节点状态

节点通过以下状态条件报告健康状况：
- `Ready`：节点就绪，可以接受 Pod。
- `MemoryPressure`：内存压力。
- `DiskPressure`：磁盘压力。
- `PIDPressure`：PID 资源压力。
- `NetworkUnavailable`：网络不可用。

## 关键机制或特性

- 节点通过 Lease 对象向 API Server 发送心跳（默认每 10 秒）。
- 节点注册（Registration）可以是自动的（kubelet 自注册）或由 Controller 创建。
- 节点可以通过标签（Labels）和污点（Taints）进行分类和调度控制。

## 使用场景与最佳实践

- 为节点设置合理的标签，便于使用 nodeSelector 或 nodeAffinity 进行调度。
- 配置 kubelet 资源预留（`--kube-reserved` 和 `--system-reserved`）。
- 监控节点资源使用率和 Pod 容量。
- 使用 Node Problem Detector 自动检测和报告节点异常。

## 参考链接

- [Node - Official Documentation](https://kubernetes.io/docs/concepts/architecture/nodes/)

## Related

[[系统基础/topic-dictionary/fundamentals/nodes.md|节点]]


<!-- risk-assessed -->
