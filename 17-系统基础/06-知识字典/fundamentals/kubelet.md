---
title: kubelet
description: kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。...
summary: kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。...
category: dictionary
tags:
- k8s
- glossary
- kubelet
- node
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 是什么
- kubelet 详解
trigger_keywords:
- kubelet
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubelet

> **英文名**: kubelet

## 概述

kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。

## 核心概念/原理

### 核心职责

- **Pod 管理**：根据 API Server 下发的 PodSpec 创建、更新和删除容器。
- **健康检查**：执行 Liveness、Readiness 和 Startup 探针。
- **资源监控**：上报节点资源使用情况和 Pod 指标。
- **日志收集**：管理容器日志文件。
- **Volume 管理**：挂载和卸载 Volume。
- **镜像管理**：通过 CRI 拉取容器镜像。

### 通信模式

kubelet 通过 API Server 获取 Pod 配置，同时向 API Server 报告节点状态和 Pod 状态。kubelet 还暴露 `/healthz`、`/metrics` 等端点供监控使用。

## 关键机制或特性

- kubelet 通过 CRI（Container Runtime Interface）与容器运行时通信。
- 支持 Static Pod（通过 manifest 目录或 URL 直接创建，不经过 API Server）。
- kubelet 的 `--config` 参数通过 KubeletConfiguration 进行配置。
- 支持 cgroup v1 和 cgroup v2。

## 使用场景与最佳实践

- 合理配置 `--max-pods` 限制单节点 Pod 数量。
- 设置 `--image-gc-high-threshold` 和 `--image-gc-low-threshold` 管理镜像垃圾回收。
- 配置 `--eviction-hard` 和 `--eviction-soft` 防止节点资源耗尽。
- 定期升级 kubelet 版本，保持与 API Server 的兼容性。

## 参考链接

- [kubelet - Official Documentation](https://kubernetes.io/docs/reference/command-line-tools-reference/kubelet/)

## Related

[[17-系统基础/06-知识字典/fundamentals/kubernetes-components.md|Kubernetes 组件]]


<!-- risk-assessed -->
