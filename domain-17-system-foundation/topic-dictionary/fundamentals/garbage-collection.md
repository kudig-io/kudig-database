---
title: Garbage Collection（垃圾回收）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- job
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Garbage Collection（垃圾回收） 是什么
- 如何 Garbage Collection（垃圾回收）
trigger_keywords:
- Garbage
- Collection
- 垃圾回收
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Garbage Collection（垃圾回收）

## 概述

垃圾回收（Garbage Collection）是 [[Kubernetes|Kubernetes]] 用于清理集群资源的各种机制的统称。它允许自动清理以下类型的资源：已终止的 Pod、已完成的 Job、没有 owner reference 的对象、未使用的容器和镜像、回收策略为 Delete 的动态供给 PersistentVolume、过期或陈旧的 CertificateSigningRequest（CSR）、以及已被删除的节点和节点 Lease 对象等。

## 核心概念/原理

- **Owner References（所有者引用）**：Kubernetes 中的许多对象通过 owner references 相互关联。它告诉控制平面哪些对象依赖于其他对象。Kubernetes 利用 owner references 在删除对象前清理相关资源，大多数情况下 owner references 是自动管理的。
- **所有权 vs 标签选择器**：所有权不同于标签和选择器机制。例如，[[Service|Service]] 通过标签确定哪些 EndpointSlice 属于它，同时这些 EndpointSlice 也会带有指向该 Service 的 owner reference，帮助 Kubernetes 各组件避免误操作不属于自己的对象。
- **跨命名空间限制**：跨命名空间的 owner reference 被设计为禁止。命名空间内的依赖对象可以指向集群范围或同命名空间的所有者；集群范围的依赖对象只能指向集群范围的所有者。v1.20+ 若检测到无效的跨命名空间 owner reference，会生成 `OwnerRefInvalidNamespace` 警告事件。

## 关键机制或特性

### 级联删除（Cascading Deletion）
删除对象时，可以控制 Kubernetes 是否自动删除其依赖对象。有两种级联删除方式：

- **前台级联删除（Foreground Cascading Deletion）**：
  - 所有者对象先进入“删除进行中”状态（设置 `deletionTimestamp` 和 `foregroundDeletion` finalizer）。
  - 控制器删除所有已知的依赖对象后，再删除所有者对象。
  - 只有 `ownerReference.blockOwnerDeletion=true` 且在垃圾回收控制器缓存中的依赖对象才会阻塞所有者删除。

- **后台级联删除（Background Cascading Deletion）**：
  - API 服务器立即删除所有者对象，垃圾回收控制器在后台异步清理依赖对象。
  - 这是 Kubernetes 的默认行为。

- **孤儿依赖（Orphan Dependents）**：
  - 删除所有者对象时，保留依赖对象而不删除。可通过特定删除选项实现。

### 容器和镜像垃圾回收（[[kubelet|kubelet]] 级别）
- **执行频率**：kubelet 每 5 分钟对未使用镜像执行一次垃圾回收，每 1 分钟对未使用容器执行一次。
- **磁盘阈值**：通过 `HighThresholdPercent` 和 `LowThresholdPercent` 控制。当磁盘使用率超过高阈值时，按最后使用时间从旧到新删除镜像，直到降至低阈值以下。
- **镜像最大存活时间**：可通过 `imageMaximumGCAge` 配置，无论磁盘使用情况如何，超过该时间的未使用镜像都会被回收。
- **容器回收变量**：
  - `MinAge`：可回收容器的最小年龄
  - `MaxPerPodContainer`：每个 Pod 可保留的死亡容器最大数量
  - `MaxContainers`：集群范围内可保留的死亡容器最大数量

### [[Finalizers|Finalizers]]
可通过 finalizers 控制垃圾回收在删除具有 owner references 的资源时的行为，确保所有必要的清理任务完成后才删除对象。

## 使用场景

- 自动清理已完成的 Job 和已终止的 Pod，释放集群资源
- 在删除 Deployment 时自动清理其创建的 ReplicaSet 和 Pod
- 在节点磁盘空间不足时，自动清理旧镜像和停止的容器
- 通过前台删除确保依赖资源在父资源删除前被正确清理

## 最佳实践/注意事项

- **避免使用外部垃圾回收工具**，因为这可能破坏 kubelet 行为，误删本应存在的容器
- 通过 `KubeletConfiguration` 资源类型调优容器和镜像垃圾回收参数
- 定期检查 `OwnerRefInvalidNamespace` 事件，排查无效的跨命名空间 owner reference
- 对于需要确保依赖资源先被清理的场景，使用前台级联删除或 finalizers

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/garbage-collection/

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
