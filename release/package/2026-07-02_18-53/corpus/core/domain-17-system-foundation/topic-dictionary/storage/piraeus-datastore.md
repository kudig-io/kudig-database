---
title: Piraeus 分布式存储
description: Piraeus Datastore 是 LINBIT 开源的 CNCF Sandbox 项目，基于 DRBD/LINSTOR 为 Kubernetes
  提供高性...
summary: Piraeus Datastore 是 LINBIT 开源的 CNCF Sandbox 项目，基于 DRBD/LINSTOR 为 Kubernetes
  提供高性...
category: dictionary
tags:
- k8s
- glossary
- storage
- replication
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Piraeus 分布式存储 是什么
- Piraeus 详解
trigger_keywords:
- Piraeus 分布式存储
- Piraeus
- dictionary
prerequisites:
- kubernetes
---



# Piraeus 分布式存储（Piraeus）

## 概述

Piraeus Datastore 是 LINBIT 开源的 CNCF Sandbox 项目，基于 DRBD/LINSTOR 为 Kubernetes 提供高性能的分布式块存储复制，实现有状态应用的同步复制和高可用。

## 核心概念/原理

- **DRBD 复制**：基于 DRBD 的块级同步/异步复制
- **LINSTOR 管理**：自动化存储资源管理
- **CNCF Sandbox**：LINBIT 主导
- **CSI 驱动**：标准 K8s CSI 集成

## 关键机制或特性

- LINSTOR CSI Driver
- 同步复制（R1）和异步复制（R2）
- 存储池管理和自动配置
- 快照和克隆
- 自动故障转移
- 加密存储卷
- 多站点复制

## 使用场景与最佳实践

- 数据库的高可用存储
- 需要同步复制的有状态应用
- 裸金属环境的分布式存储
- 替代 Ceph RBD 的轻量方案
- 多站点的存储复制

## 参考链接

- https://piraeus.io/
- https://github.com/piraeusdatastore/piraeus-operator

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/rook.md|Rook]]
- [[domain-17-system-foundation/topic-dictionary/storage/longhorn.md|Longhorn]]
- [[domain-17-system-foundation/topic-dictionary/storage/openebs.md|OpenEBS]]
