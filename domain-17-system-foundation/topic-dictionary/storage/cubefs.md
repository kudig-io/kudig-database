---
title: CubeFS 分布式文件系统
description: 'CubeFS（原 CFS）是京东开源的 CNCF 孵化项目，云原生分布式文件系统，支持 POSIX/SDK/S3 多协议访问，为 AI 训练和大数据提供高吞吐的...'
category: dictionary
tags:
- k8s
- glossary
- storage
- filesystem
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CubeFS 分布式文件系统 是什么
- CubeFS 详解
trigger_keywords:
- CubeFS 分布式文件系统
- CubeFS
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# CubeFS 分布式文件系统（CubeFS）

## 概述

CubeFS（原 CFS）是京东开源的 CNCF 孵化项目，云原生分布式文件系统，支持 POSIX/SDK/S3 多协议访问，为 AI 训练和大数据提供高吞吐的共享文件存储。

## 核心概念/原理

- **分布式文件**：POSIX 兼容的分布式文件系统
- **多协议**：POSIX/SDK/S3/HDFS 访问
- **CNCF 孵化**：京东/OPPO 等联合推动
- **AI 优化**：为 AI 训练优化的大文件吞吐

## 关键机制或特性

- Master/MetaNode/DataNode/ObjectNode 架构
- 多副本和纠删码（Erasure Coding）
- 元数据分区和水平扩展
- S3 兼容 API
- 快照和克隆
- 多租户配额管理
- CSI 驱动

## 使用场景与最佳实践

- AI 训练的共享文件存储
- 大数据分析的分布式文件系统
- 容器化应用的高性能存储
- 多租户文件存储平台
- 对象存储和文件存储的统一

## 参考链接

- https://cubefs.io/
- https://github.com/cubefs/cubefs

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/fluid.md|Fluid]]
- [[domain-17-system-foundation/topic-dictionary/storage/minio.md|MinIO]]
