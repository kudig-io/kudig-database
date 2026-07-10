---
title: OpenEBS 容器存储
description: OpenEBS 是 Maya Data 开源的 CNCF Sandbox 项目，为 Kubernetes 提供容器附加存储（CAS），支持多种存储引擎（Loca...
summary: OpenEBS 是 Maya Data 开源的 CNCF Sandbox 项目，为 Kubernetes 提供容器附加存储（CAS），支持多种存储引擎（Loca...
category: dictionary
tags:
- k8s
- glossary
- storage
- csi
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
- OpenEBS 容器存储 是什么
- OpenEBS 详解
trigger_keywords:
- OpenEBS 容器存储
- OpenEBS
- dictionary
prerequisites:
- kubernetes
---



# OpenEBS 容器存储（OpenEBS）

## 概述

OpenEBS 是 Maya Data 开源的 CNCF Sandbox 项目，为 Kubernetes 提供容器附加存储（CAS），支持多种存储引擎（Local PV、Replicated PV、ZFS Local PV），是有状态应用的存储方案。

## 核心概念/原理

- **容器附加存储**：将存储引擎容器化，与应用同生命周期管理
- **多引擎**：Local PV / Replicated PV（Mayastor）/ ZFS Local PV / LVM Local PV
- **Kubernetes 原生**：通过 CSI 驱动集成
- **CNCF Sandbox**：活跃的容器存储社区

## 关键机制或特性

- Local PV Hostpath / Device 模式
- Mayastor：基于 SPDK/NVMe-oF 的高性能复制引擎
- ZFS Local PV：利用 ZFS 特性的本地存储
- LVM Local PV：基于 LVM 的本地卷管理
- 快照和克隆支持
- CStor（已弃用，迁移至 Mayastor）

## 使用场景与最佳实践

- 有状态应用（数据库/消息队列）的持久化存储
- 需要本地存储高性能 I/O 的场景
- 云和裸金属环境的统一存储方案
- 开发/测试环境的快速存储配置
- 存储数据的快照和克隆

## 参考链接

- https://openebs.io/
- https://github.com/openebs/openebs

## Related

- [[系统基础/知识字典/storage/rook.md|Rook]]
- [[系统基础/知识字典/storage/longhorn.md|Longhorn]]
- [[系统基础/知识字典/storage/ceph.md|Ceph]]
