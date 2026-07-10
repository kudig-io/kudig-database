---
title: 容器网络接口
description: CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使
  Kubernet...
summary: CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使 Kubernet...
category: dictionary
tags:
- k8s
- glossary
- cni
- networking
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器网络接口 是什么
- CNI (Container Network Interface) 详解
trigger_keywords:
- 容器网络接口
- CNI (Container Network Interface)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器网络接口

> **英文名**: CNI (Container Network Interface)

## 概述

CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使 Kubernetes 能够使用各种网络插件实现 Pod 间通信。

## 核心概念/原理

### 核心概念

- **CNI 插件**：实现 CNI 规范的网络软件，如 Calico、Cilium、Flannel、Weave 等。
- **CNI 配置**：通过 JSON 配置文件定义网络拓扑和 IP 分配策略。
- **CNI 执行流程**：
  1. kubelet 通过 CRI 创建容器。
  2. 容器运行时调用 CNI 插件。
  3. CNI 插件配置网络接口和路由。

### 主流 CNI 插件

| 插件 | 数据面 | 网络策略 | 特点 |
|------|--------|---------|------|
| Calico | BGP/VXLAN | 支持 | 纯三层路由，性能优秀 |
| Cilium | eBPF | 支持 | 内核旁路，高性能 |
| Flannel | VXLAN | 不支持 | 简单轻量 |
| Weave | mesh | 支持 | 加密通信 |

## 关键机制或特性

- CNI 由 CNCF 维护，是容器网络的事实标准。
- CNI 配置文件位于 `/etc/cni/net.d/` 目录。
- 一个节点可以有多个 CNI 配置，按文件名排序选择。

## 使用场景与最佳实践

- 生产环境推荐 Calico 或 Cilium，功能完整且性能优秀。
- 大规模集群考虑 Cilium 的 eBPF 数据面获得更好性能。
- 确保 CNI 版本与 Kubernetes 版本兼容。
- 监控 CNI 的 IP 分配情况和网络延迟。

## 参考链接

- [CNI (Container Network Interface) - Official Documentation](https://www.cni.dev/)

## Related

[[实体/cilium.md|Cilium]] | [[实体/cni-plugins.md|CNI Plugins]]


<!-- risk-assessed -->
