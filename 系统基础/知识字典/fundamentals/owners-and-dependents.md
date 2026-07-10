---
title: 所有者和依赖者
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- daemonset
- job
- cronjob
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 所有者和依赖者 是什么
- 如何 所有者和依赖者
trigger_keywords:
- 所有者和依赖者
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 所有者和依赖者

## 概述

在 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 中，一些对象是所有者（owners），而另一些对象是它们的依赖者（dependents）。例如，[[ReplicaSet|ReplicaSet]] 是一组 Pod 的所有者。所有权与标签和选择器机制不同，它帮助 Kubernetes 的不同部分避免干扰它们不控制的对象。

## 核心概念/原理

### Owner References

依赖对象具有 `metadata.ownerReferences` 字段，用于引用其所有者对象。一个有效的所有者引用包含对象名称和 UID，且必须与依赖对象位于同一命名空间。Kubernetes 会自动为 ReplicaSet、[[DaemonSet|DaemonSet]]、Deployment、Job、[[CronJob|CronJob]] 和 ReplicationController 等对象的依赖资源设置此字段。

虽然可以手动更改此字段，但通常不需要，可以让 Kubernetes 自动管理这些关系。

### blockOwnerDeletion

依赖对象还有一个 `ownerReferences.blockOwnerDeletion` 字段，这是一个布尔值，控制特定依赖者是否可以阻止垃圾回收器删除其所有者对象。如果控制器设置了 `metadata.ownerReferences`，Kubernetes 会自动将此字段设置为 `true`。用户也可以手动设置此字段来控制哪些依赖者阻止垃圾回收。

Kubernetes 准入控制器根据所有者的删除权限控制用户更改此字段的访问权限，防止未授权用户延迟所有者对象的删除。

### 跨命名空间限制

- 跨命名空间的所有者引用被设计为禁止的。
- 命名空间范围的依赖者可以指定集群范围或命名空间范围的所有者。
- 命名空间范围的所有者必须与依赖者位于同一命名空间。
- 集群范围的依赖者只能指定集群范围的所有者。自 v1.20+ 起，如果集群范围的依赖者指定了命名空间类型的所有者，将被视为无法解析的所有者引用，无法进行垃圾回收。

## 关键机制或特性

### 所有权与 Finalizers

当用户请求删除资源时，API 服务器允许管理控制器处理该资源的任何 Finalizer 规则。例如，删除仍在被 Pod 使用的 PersistentVolume 不会立即发生，因为存在 `kubernetes.io/pv-protection` Finalizer。

Kubernetes 在使用前台级联删除或孤立删除时也会向所有者资源添加 Finalizer：
- **前台删除（Foreground deletion）**：添加 `foreground` Finalizer，控制器必须在删除所有者之前删除那些 `ownerReferences.blockOwnerDeletion=true` 的依赖资源。
- **孤立删除（Orphan deletion）**：添加 `orphan` Finalizer，控制器在删除所有者对象后会忽略依赖资源。

## 使用场景

- 确保删除 Deployment 时，其创建的 ReplicaSet 和 Pod 被正确清理。
- 通过设置 `blockOwnerDeletion` 控制依赖资源是否阻止所有者的删除。
- 在自定义控制器中建立资源之间的生命周期关系。

## 最佳实践/注意事项

- 通常不需要手动设置 `ownerReferences`，应让 Kubernetes 控制器自动管理。
- 避免创建跨命名空间的所有者引用，这会导致依赖资源无法被正确垃圾回收。
- 当对象删除卡住时，检查所有者和依赖者上的 `ownerReferences` 和 `finalizers` 是排查问题的关键步骤。

## 参考链接

- [Owners and Dependents - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/owners-dependents/)

## Related

- [[系统基础/知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[系统基础/知识字典/fundamentals/annotations.md|注解]]
- [[系统基础/知识字典/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
