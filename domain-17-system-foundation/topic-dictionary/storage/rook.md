---
title: Rook
description: Rook 是 CNCF 毕业项目，为 Kubernetes 提供云原生存储编排平台。它通过 Operator 模式自动化部署和管理分布式存储系统（Ceph、Ed...
summary: Rook 是 CNCF 毕业项目，为 Kubernetes 提供云原生存储编排平台。它通过 Operator 模式自动化部署和管理分布式存储系统（Ceph、Ed...
category: dictionary
tags:
- k8s
- glossary
- rook
- storage
- operator
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
- Rook 是什么
- Rook 详解
trigger_keywords:
- Rook
- dictionary
prerequisites:
- kubectl-basics
---



# Rook

> **英文名**: Rook

## 概述

Rook 是 CNCF 毕业项目，为 Kubernetes 提供云原生存储编排平台。它通过 Operator 模式自动化部署和管理分布式存储系统（Ceph、EdgeFS 等），让存储系统在 Kubernetes 中像使用云服务一样简单。

## 核心概念/原理

### 核心架构

- **Rook Operator**：管理存储集群的生命周期（安装、升级、扩缩、故障恢复）。
- **Ceph Cluster**：Rook 管理的分布式存储后端（最常用）。
- **CSI Driver**：Rook-Ceph CSI 提供 PV 动态制备。

### Rook-Ceph 存储能力

| 类型 | K8s 资源 | 说明 |
|------|----------|------|
| Block (RBD) | ReadWriteOnce PV | 数据库、有状态应用 |
| Filesystem (CephFS) | ReadWriteMany PV | 共享文件存储 |
| Object (RGW) | S3 兼容 API | 对象存储、备份目标 |

## 关键机制或特性

- **全自动运维**：OSD 故障自动恢复、数据自动重平衡。
- **弹性扩缩**：动态添加/移除 OSD 节点。
- **加密**：支持 OSD 级别的静态加密（encryption at rest）。
- **Dashboard**：内置 Ceph Dashboard 监控存储健康。
- 支持快照（Snapshot）和克隆（Clone）功能。

## 使用场景与最佳实践

- 需要 Kubernetes 原生存储能力时优先考虑 Rook-Ceph。
- 确保至少有 3 个 OSD 节点实现数据冗余。
- 为 RBD 和 CephFS 分别创建 StorageClass。
- 监控 Ceph 集群健康状态（HEALTH_OK/WARN/ERR）。
- 配置 Pool 的副本数和故障域（failureDomain）。

## 参考链接

- [Rook Official](https://rook.io/docs/rook/latest/)

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/csi.md|CSI]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern.md|Operator Pattern]]
