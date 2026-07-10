---
title: Leases（租约）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- scheduler
- controller-manager
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Leases（租约） 是什么
- 如何 Leases（租约）
trigger_keywords:
- Leases
- 租约
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Leases（租约）

## 概述

Lease（租约）是分布式系统中用于锁定共享资源和协调集合成员活动的机制。在 [[Kubernetes|Kubernetes]] 中，Lease 对象属于 `coordination.k8s.io` API 组，被用于系统级关键能力，如节点心跳（node heartbeats）和组件级领导者选举（leader election）。

## 核心概念/原理

- **Lease 对象**：一种轻量级的 Kubernetes 资源，通过 `spec.holderIdentity` 标识持有者，通过 `spec.renewTime` 记录最近一次续约时间。
- **节点心跳**：每个 Node 对应一个同名 Lease，位于 `kube-node-lease` 命名空间。[[kubelet|kubelet]] 每次心跳实际上都是对该 Lease 对象的更新请求。控制平面通过 `renewTime` 判断节点是否可用。
- **领导者选举**：Kubernetes 的控制平面组件（如 `kube-controller-manager`、`kube-scheduler`）在高可用（HA）配置下使用 Lease 确保同一时刻只有一个实例处于活跃状态，其余实例待命。
- **API 服务器身份**：自 v1.26（Beta，默认启用）起，每个 `kube-apiserver` 实例通过 Lease API 向系统发布自身身份。Lease 位于 `kube-system` 命名空间，名称格式为 `apiserver-<sha256-hash>`，可通过标签 `apiserver.kubernetes.io/identity=kube-apiserver` 筛选。过期 Lease 会在 1 小时后由新实例垃圾回收。可通过关闭 `APIServerIdentity` 特性门控禁用此行为。

## 关键机制或特性

- **轻量协调**：相比直接更新 Node 的 `.status`，Lease 更新更轻量，可减少 API 服务器负载。
- **工作负载可用**：用户的工作负载也可以定义自己的 Lease 用法。例如自定义控制器可通过 Lease 实现多副本之间的领导者选举。
- **命名规范**：建议 Lease 名称与产品或组件明显关联（如组件名为 Example Foo，则 Lease 名为 `example-foo`）。若可能部署多实例，应使用名称前缀并添加机制（如 Deployment 名称的哈希）避免冲突。

## 使用场景

- kubelet 向控制平面报告节点存活状态
- 高可用控制平面组件的领导者选举
- 发现当前控制平面中运行的 kube-apiserver 实例数量
- 自定义控制器或 Operator 实现分布式锁和领导者选举

## 最佳实践/注意事项

- 自定义 Lease 的名称应具有明确的产品关联性，避免不同软件之间的命名冲突
- 若组件可能被部署多实例，设计 Lease 名称时考虑冲突避免机制
- 跨命名空间的 owner reference 是被禁止的；namespaced 的依赖对象必须与所有者在同一命名空间
- 定期检查 `OwnerRefInvalidNamespace` 事件，排查无效的跨命名空间 owner reference

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/leases/

## Related

- [[系统基础/知识字典/workloads/pod.md|Pod]]
- [[系统基础/知识字典/fundamentals/container.md|Container]]
- [[系统基础/知识字典/fundamentals/node.md|Node]]
- [[系统基础/知识字典/fundamentals/namespace.md|Namespace]]
- [[系统基础/知识字典/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
