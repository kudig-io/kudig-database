---
title: etcd
description: etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、...
summary: etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、...
category: dictionary
tags:
- k8s
- glossary
- etcd
- control-plane
- storage
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd 是什么
- etcd 详解
trigger_keywords:
- etcd
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# etcd

> **英文名**: etcd

## 概述

etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、Secret 等所有资源对象）都持久化在 etcd 中。

## 核心概念/原理

### 核心特性

- **强一致性**：基于 Raft 共识算法，保证所有读取返回最新数据。
- **Watch 机制**：支持对 key 或 key 前缀的变更监听，是 Kubernetes 事件驱动架构的基础。
- **事务支持**：支持多 key 的原子操作。
- **MVCC 存储**：使用多版本并发控制，保留 key 的历史版本。

### 在 Kubernetes 中的角色

API Server 是唯一直接与 etcd 通信的组件。所有 Kubernetes 对象通过 API Server 读写 etcd。etcd 中的数据变更触发控制器和 Informer 的响应。

## 关键机制或特性

- etcd 集群推荐至少 3 个成员以实现容错（可容忍 1 个节点故障）。
- 5 个成员的集群可容忍 2 个节点故障，适合大规模生产环境。
- 需要定期执行 compaction（压缩历史版本）和 defragmentation（回收空间）。
- 备份策略：定期执行 `etcdctl snapshot save` 并存储在异地。

## 使用场景与最佳实践

- **性能**：使用 SSD 存储，避免网络延迟；大规模集群考虑独立 etcd 集群。
- **安全**：启用 TLS 加密所有 etcd 通信（peer 和 client）。
- **备份**：实施自动化备份策略，定期验证备份可恢复性。
- **监控**：关注 WAL fsync 延迟、backend commit 延迟等关键指标。
- **版本**：Kubernetes 对 etcd 版本有严格要求，参见兼容性矩阵。

## 参考链接

- [etcd - Official Documentation](https://etcd.io/docs/)

## Related

[[系统基础/知识字典/fundamentals/storage-versions.md|存储版本]]


<!-- risk-assessed -->
