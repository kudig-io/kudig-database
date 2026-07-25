---
title: 控制器管理器
description: kube-controller-manager 是 Kubernetes 控制平面中运行各种控制器的组件。每个控制器都是一个独立的控制循环，持续比较集群的当前状...
summary: kube-controller-manager 是 Kubernetes 控制平面中运行各种控制器的组件。每个控制器都是一个独立的控制循环，持续比较集群的当前状...
category: dictionary
tags:
- k8s
- glossary
- controller-manager
- control-plane
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 控制器管理器 是什么
- kube-controller-manager 详解
trigger_keywords:
- 控制器管理器
- kube-controller-manager
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 控制器管理器

> **英文名**: kube-controller-manager

## 概述

kube-controller-manager 是 Kubernetes 控制平面中运行各种控制器的组件。每个控制器都是一个独立的控制循环，持续比较集群的当前状态与期望状态，并在偏差时采取纠正措施。

## 核心概念/原理

### 内置控制器

kube-controller-manager 运行的核心控制器包括：

- **Node Controller**：监控节点状态，处理节点加入/离开/故障。
- **Replication Controller**：维护 ReplicaSet 中 Pod 的副本数。
- **Deployment Controller**：管理 Deployment 的滚动更新和回滚。
- **ServiceAccount Controller**：为新命名空间创建默认 ServiceAccount。
- **Namespace Controller**：处理命名空间的删除及其资源的级联清理。
- **Job Controller**：管理 Job 的执行。
- **EndpointSlice Controller**：维护 Service 和 Pod 之间的映射关系。

### 工作原理

每个控制器独立运行一个 Reconcile 循环：读取当前状态 → 计算差异 → 执行纠正操作 → 更新状态。

## 关键机制或特性

- 控制器通过 Informer 机制缓存集群状态，减少对 API Server 的压力。
- 支持水平扩展：通过 Leader Election 确保同一时间只有一个活跃的 Controller Manager。
- 控制器之间松耦合，各自独立运行，互不干扰。

## 使用场景与最佳实践

- 监控控制器的 reconcile 延迟和错误率。
- 调整 `--concurrent-*-syncs` 参数控制控制器的并发度。
- 定期检查控制器日志，排查 reconcile 失败的资源。

## 参考链接

- [kube-controller-manager - Official Documentation](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-controller-manager/)

## Related

[[17-系统基础/06-知识字典/fundamentals/controllers.md|控制器]]


<!-- risk-assessed -->
