---
title: 控制器管理器
description: kube-controller-manager 是 Kubernetes 控制平面组件，负责运行各种控制器（Controller）的循环逻辑。每个控制器是独立的...
summary: kube-controller-manager 是 Kubernetes 控制平面组件，负责运行各种控制器（Controller）的循环逻辑。每个控制器是独立的...
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

kube-controller-manager 是 Kubernetes 控制平面组件，负责运行各种控制器（Controller）的循环逻辑。每个控制器是独立的 goroutine，通过 apiserver 监听资源变化并执行调谐（reconcile）操作。

## 核心概念/原理

### 内置控制器

| 控制器 | 职责 |
|--------|------|
| Node Controller | 监控节点健康状态 |
| Deployment Controller | 管理 Deployment 和 ReplicaSet |
| ReplicaSet Controller | 维持 Pod 副本数 |
| EndpointSlice Controller | 维护 Service Endpoints |
| ServiceAccount Controller | 为新命名空间创建默认 SA |
| Job Controller | 管理 Job 生命周期 |
| Namespace Controller | 处理命名空间删除 |
| PV Controller | 管理 PersistentVolume 绑定 |

## 关键机制或特性

- 所有控制器共享一个进程，通过 `--controllers` 标志控制启用/禁用。
- 控制器采用 watch + reconcile 模式，持续将系统状态推向期望状态。
- leader election 确保多副本部署时只有一个活跃实例。

## 使用场景与最佳实践

- 通过 `--concurrent-*` 参数调优控制器的并发处理能力。
- 监控 `workqueue_depth` 和 `workqueue_latency` 指标检测控制器性能。
- 自定义控制器推荐使用 controller-runtime 或 Kubebuilder 框架。

## 参考链接

- [kube-controller-manager - Kubernetes Docs](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-controller-manager/)

## Related

- [[系统基础/topic-dictionary/fundamentals/kube-apiserver.md|Kube-apiserver]]
- [[系统基础/topic-dictionary/fundamentals/kube-scheduler.md|Kube-scheduler]]
- [[系统基础/topic-dictionary/workloads/deployment.md|Deployment]]
- [[系统基础/topic-dictionary/workloads/replicaset.md|ReplicaSet]]
- [[系统基础/topic-dictionary/platform-engineering/operator-pattern.md|Operator Pattern]]


<!-- risk-assessed -->
