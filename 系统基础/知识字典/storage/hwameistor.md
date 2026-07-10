---
title: HwameiStor 本地存储
description: HwameiStor 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供高可用本地存储管理，自动管理本地磁盘并通过数据...
summary: HwameiStor 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供高可用本地存储管理，自动管理本地磁盘并通过数据...
category: dictionary
tags:
- k8s
- glossary
- storage
- local-storage
- ha
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HwameiStor 本地存储 是什么
- HwameiStor 详解
trigger_keywords:
- HwameiStor 本地存储
- HwameiStor
- dictionary
prerequisites:
- kubernetes
---



# HwameiStor 本地存储（HwameiStor）

## 概述

HwameiStor 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供高可用本地存储管理，自动管理本地磁盘并通过数据复制实现本地卷的高可用。

## 核心概念/原理

- **本地存储管理**：自动发现和管理节点本地磁盘
- **高可用**：本地卷的数据复制和故障转移
- **CNCF Sandbox**：DaoCloud 主导
- **CSI 驱动**：标准 K8s CSI 集成

## 关键机制或特性

- LocalDiskNode 自动发现本地磁盘
- LocalVolume 本地卷管理
- 数据复制（同步/异步）
- 卷迁移（节点故障时自动迁移）
- 磁盘健康检查
- 存储池管理
- 卷扩容

## 使用场景与最佳实践

- 本地磁盘的高可用管理
- 数据库的本地存储方案
- 存储成本优化（利用本地磁盘）
- 边缘设备的存储管理
- 需要高 IOPS 的有状态应用

## 参考链接

- https://hwameistor.io/
- https://github.com/hwameistor/hwameistor

## Related

- [[系统基础/知识字典/storage/openebs.md|OpenEBS]]
- [[系统基础/知识字典/storage/longhorn.md|Longhorn]]
- [[系统基础/知识字典/storage/rook.md|Rook]]
