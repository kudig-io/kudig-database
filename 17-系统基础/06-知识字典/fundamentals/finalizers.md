---
title: Finalizers
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Finalizers 是什么
- 如何 Finalizers
trigger_keywords:
- Finalizers
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Finalizers

## 概述

Finalizers 是带有命名空间限制的键，用于告诉 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 在完全删除标记为删除的资源之前等待特定条件满足。Finalizers 会通知控制器清理被删除对象所拥有的资源。

## 核心概念/原理

### Finalizers 如何工作

当用户请求删除一个带有 Finalizers 的对象时，[[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 服务器会：

1. 在对象上添加 `metadata.deletionTimestamp` 字段，标记对象已被请求删除。
2. 阻止对象被移除，直到其 `metadata.finalizers` 字段中的所有项都被移除。
3. 返回 `202 Accepted` 状态码。

管理该 Finalizer 的控制器会注意到 `deletionTimestamp` 的更新，然后尝试满足该资源指定 Finalizer 的要求。每次满足一个 Finalizer 条件，控制器就会从资源的 `finalizers` 字段中移除该键。当 `finalizers` 字段为空时，Kubernetes 会自动删除该对象。

### 常见示例

`kubernetes.io/pv-protection` 是一个常见的 Finalizer，用于防止 PersistentVolume 被意外删除。当 PersistentVolume 正在被 Pod 使用时，Kubernetes 会添加此 Finalizer。如果用户尝试删除该 PV，它会进入 `Terminating` 状态，但控制器无法删除它，因为 Finalizer 仍然存在。只有当 Pod 停止使用该 PV 后，Kubernetes 才会清除该 Finalizer，然后控制器才会删除该卷。

## 关键机制或特性

- **防止删除**：Finalizers 可用于阻止非托管资源被删除，确保在对象完全删除前执行必要的清理工作。
- **与垃圾回收的关系**：Finalizers 是控制资源垃圾回收的重要手段。
- **自定义 Finalizer 命名规范**：自定义 Finalizer 名称必须是公开限定的格式，如 `example.com/finalizer-name`。Kubernetes 会强制此格式，API 服务器会拒绝不符合规范的写入。

### 删除后的限制

- 对象被请求删除后，`.metadata.finalizers` 字段会被立即限制修改：可以移除现有 Finalizer，但不能添加新的 Finalizer，也不能修改 `deletionTimestamp`。
- 删除请求发出后，对象无法"复活"，唯一的方法是删除它并创建一个相似的新对象。

### 与 Owner References 的关系

Finalizers 有时会阻止依赖对象的删除，从而导致目标所有者对象长时间处于未完全删除的状态。此时应检查目标所有者对象和依赖对象上的 Finalizers 和 Owner References 以排查原因。

**注意**：当对象卡在删除状态时，避免手动移除 Finalizers 以强制继续删除。Finalizers 通常是有目的添加的，强行移除可能导致集群问题。只有在理解 Finalizer 的目的并通过其他方式完成其任务后，才应手动移除。

## 使用场景

- 在删除自定义资源前清理外部基础设施（如云资源、数据库记录）。
- 防止仍在使用的存储卷被意外删除。
- 实现级联删除的前台清理逻辑。

## 最佳实践/注意事项

- 设计自定义控制器时，合理设置 Finalizer 以确保资源清理的完整性。
- 遇到资源长期 Terminating 时，优先排查依赖关系和控制器日志，而不是直接移除 Finalizer。
- 自定义 Finalizer 必须使用符合规范的限定名称。

## 参考链接

- [Finalizers - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/finalizers/)

## Related

- [[17-系统基础/06-知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[17-系统基础/06-知识字典/fundamentals/annotations.md|注解]]
- [[17-系统基础/06-知识字典/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
