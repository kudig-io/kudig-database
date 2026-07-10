---
title: 存储版本
description: '# 存储版本'
summary: '# 存储版本'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- crd
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储版本 是什么
- 如何 存储版本
trigger_keywords:
- 存储版本
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 存储版本

## 概述

[[Kubernetes|Kubernetes]]es API|Kubernetes API]] 服务器将对象存储在 [[系统基础/topic-dictionary/fundamentals/etcd.md|etcd]]（或兼容的键值存储）中。每个对象使用特定版本的 API 类型进行序列化。Kubernetes 使用"存储版本"（storage version）这一术语来描述对象在集群中的实际存储方式。

## 核心概念/原理

### 存储版本与 API 版本

- **存储版本**：对象在存储后端（如 etcd）中的实际二进制编码格式。
- **API 版本**：用户与 Kubernetes API 交互时使用的版本。

Kubernetes 支持自动转换。例如，对于 HorizontalPodAutoscaler，用户可以使用 v1 或 v2 版本的 API 进行交互，Kubernetes 负责在 API 调用之间进行转换，客户端无需关心实际序列化使用的是哪个版本。

对象的版本与存储版本完全独立。例如，同一资源的 `v1alpha1` 和 `v1beta1` API 对象在存储中的编码可能是相同的，只要存储版本没有更新。

### 存储版本与资源映射

每个资源在任意时刻只有一个活跃的存储版本。任何对对象的写入都会以该存储版本存储。存储版本可以更新，这意味着对象可以存储在不同的版本中，但单个对象在任何时候只存储在一个版本中。

从 API 服务器读取时，存储数据会转换为对象的 API 表示形式。因此，旧的存储版本可以无限期存在，只要不对对象进行更新。而写入操作会在更新时将存储对象转换为新的表示形式。

### 自定义资源的存储版本

自定义资源（CRD）必须将某个特定版本设置为存储版本。该版本定义的 schema 将用作存储层中资源的编码格式。

**示例**：一个 `crontabs` 的 CRD 可能定义 `v1beta1` 为 `storage: true`，`v1` 为 `storage: false`。此时 `v1beta1` 的 schema 用于存储层编码。如果 `v1` 中有一个 `time` 字段不在 `v1beta1` 的 schema 中，那么该字段实际上无法被持久化存储。

当修改存储版本后，任何新的创建或更新操作都会使用新定义的存储版本。而读取/监听操作只是将对象从旧存储版本转换过来，不会影响底层存储。

## 关键机制或特性

### 与静态加密的关系

集群可以使用工具对静态存储进行加密（特别是 Secret）。API 服务器在从存储中检索数据时负责解密。因此，API 服务器必须拥有对应存储版本的密钥才能正确解码对象。

### 迁移到新存储版本

当管理员想要移除 CRD 的旧 API 版本时，必须确保所有对象都已从旧的存储版本迁移。否则，移除版本后可能无法读取仍使用旧存储版本的对象。同样，在密钥轮换时，旧的加密密钥也必须保留，直到所有对象至少被写入一次。

Kubernetes 提供"存储版本迁移"机制，可以在无需手动干预的情况下确保所有对象使用新的存储版本。

## 使用场景

- 升级自定义资源定义时，逐步淘汰旧 API 版本。
- 在启用静态加密或进行密钥轮换时，确保所有对象都使用新的存储版本。
- 理解 API 弃用策略时，区分"停止提供 API 版本"和"迁移存储版本"这两个概念。

## 最佳实践/注意事项

- 在移除 CRD 的旧 API 版本前，务必确认所有对象已完成存储版本迁移。
- 密钥轮换后，旧密钥不能立即删除，需确保所有对象都已重新写入。
- 定期检查集群的存储版本状态，使用存储版本迁移工具进行主动维护。

## 参考链接

- [Storage Versions - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/storage-version/)

## Related

- [[系统基础/topic-dictionary/workloads/pod.md|Pod]]
- [[系统基础/topic-dictionary/fundamentals/container.md|Container]]
- [[系统基础/topic-dictionary/fundamentals/node.md|Node]]
- [[系统基础/topic-dictionary/fundamentals/namespace.md|Namespace]]
- [[系统基础/topic-dictionary/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
