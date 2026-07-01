---
title: Longhorn
description: Longhorn 是 SUSE（原 Rancher）开源的 Kubernetes 原生分布式块存储系统，现为 CNCF 孵化项目。它以轻量、易用和自动化著称，特...
summary: Longhorn 是 SUSE（原 Rancher）开源的 Kubernetes 原生分布式块存储系统，现为 CNCF 孵化项目。它以轻量、易用和自动化著称，特...
category: dictionary
tags:
- k8s
- glossary
- longhorn
- storage
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Longhorn 是什么
- Longhorn 详解
trigger_keywords:
- Longhorn
- dictionary
prerequisites:
- kubectl-basics
---



# Longhorn

> **英文名**: Longhorn

## 概述

Longhorn 是 SUSE（原 Rancher）开源的 Kubernetes 原生分布式块存储系统，现为 CNCF 孵化项目。它以轻量、易用和自动化著称，特别适合中小规模集群和边缘场景的持久化存储需求。

## 核心概念/原理

### 核心特性

- **微服务架构**：每个 Volume 有独立的 Engine 和 Replica 进程。
- **增量快照与备份**：支持增量快照和备份到 S3/NFS。
- **自动恢复**：Replica 故障自动重建。
- **DR Volume**：跨集群灾备卷。

### 与其他存储方案对比

| 特性 | Longhorn | Rook-Ceph | NFS |
|------|----------|-----------|-----|
| 复杂度 | 低 | 高 | 中 |
| 适用规模 | 中小集群 | 大集群 | 任意 |
| 数据本地性 | 强 | 强 | 弱 |
| RWX 支持 | NFS-based | CephFS | 原生 |

## 关键机制或特性

- Longhorn UI 提供可视化存储管理。
- 支持 Volume 的在线扩容和迁移。
- 自动创建 Volume 的定期快照计划。
- 支持 Volume 的加密和访问控制。
- 通过 StorageClass 实现 PV 动态制备。

## 使用场景与最佳实践

- 中小集群或边缘场景优先考虑 Longhorn。
- 配置至少 3 个 Replica 确保数据可靠性。
- 启用自动快照和备份策略。
- 监控 Longhorn 的 Engine/Replica 状态。
- 使用 RecurringJob 自动化快照和备份任务。

## 参考链接

- [Longhorn Official](https://longhorn.io/docs/)

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/rook.md|Rook]]
- [[domain-17-system-foundation/topic-dictionary/storage/csi.md|CSI]]
- [[domain-17-system-foundation/topic-dictionary/workloads/statefulset.md|StatefulSet]]
