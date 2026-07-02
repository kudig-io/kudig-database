---
title: Ceph
description: Ceph 是最广泛使用的开源分布式存储系统，提供块存储（RBD）、对象存储（RGW）和文件存储（CephFS）三种接口。通过 Rook 集成到
  Kubernet...
summary: Ceph 是最广泛使用的开源分布式存储系统，提供块存储（RBD）、对象存储（RGW）和文件存储（CephFS）三种接口。通过 Rook 集成到 Kubernet...
category: dictionary
tags:
- k8s
- glossary
- ceph
- storage
- distributed-storage
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ceph 是什么
- Ceph 详解
trigger_keywords:
- Ceph
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Ceph

> **英文名**: Ceph

## 概述

Ceph 是最广泛使用的开源分布式存储系统，提供块存储（RBD）、对象存储（RGW）和文件存储（CephFS）三种接口。通过 Rook 集成到 Kubernetes 中，是大规模集群存储的首选方案。

## 核心概念/原理

### 核心架构

| 组件 | 功能 |
|------|------|
| OSD | 对象存储守护进程，管理物理磁盘 |
| MON | 集群状态监控和 CRUSH Map 维护 |
| MDS | CephFS 元数据服务 |
| MGR | 集群管理和 Dashboard |
| RGW | S3/Swift 兼容的对象存储网关 |

### CRUSH 算法

CRUSH（Controlled Replication Under Scalable Hashing）决定数据如何分布到 OSD，无需中心化的元数据查询。

## 关键机制或特性

- **RBD（块设备）**：Kubernetes PV 的主要来源，支持快照和克隆。
- **CephFS（文件系统）**：支持 ReadWriteMany 的共享存储。
- **RGW（对象网关）**：S3 兼容接口，适合备份和大数据。
- **数据冗余**：副本（Replicated）或纠删码（Erasure Coding）。
- **自动恢复**：OSD 故障后自动重平衡数据。

## 使用场景与最佳实践

- 通过 Rook-Ceph Operator 在 K8s 中部署和管理 Ceph 集群。
- 为不同工作负载创建不同的 Pool 和 StorageClass。
- 数据库使用 RBD（块存储）获得最佳 IOPS。
- 共享文件存储使用 CephFS。
- 监控 Ceph 集群健康状态：`ceph health detail`。

## 参考链接

- [Ceph Official](https://docs.ceph.com/)

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/rook.md|Rook]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/csi.md|CSI]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume.md|Volume]]


<!-- risk-assessed -->
