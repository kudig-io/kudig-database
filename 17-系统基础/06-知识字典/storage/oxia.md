---
title: Oxia 元数据协调
description: Oxia 是 DataStax 开源的分布式元数据协调服务，设计为 Apache Pulsar 的 ZooKeeper 替代品，提供高性能的分布式锁、序列号和元...
summary: Oxia 是 DataStax 开源的分布式元数据协调服务，设计为 Apache Pulsar 的 ZooKeeper 替代品，提供高性能的分布式锁、序列号和元...
category: dictionary
tags:
- k8s
- glossary
- storage
- metadata
- coordination
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Oxia 元数据协调 是什么
- Oxia 详解
trigger_keywords:
- Oxia 元数据协调
- Oxia
- dictionary
prerequisites:
- kubernetes
---



# Oxia 元数据协调（Oxia）

## 概述

Oxia 是 DataStax 开源的分布式元数据协调服务，设计为 Apache Pulsar 的 ZooKeeper 替代品，提供高性能的分布式锁、序列号和元数据管理。

## 核心概念/原理

- **ZooKeeper 替代**：专为云原生设计的元数据协调服务
- **高性能**：基于 RocksDB + Raft 的高吞吐实现
- **Pulsar 优化**：Apache Pulsar 的下一代元数据后端
- **DataStax 开源**：活跃的分布式系统社区

## 关键机制或特性

- Key-Value 存储（Get/Put/Delete）
- 分布式锁（Lock/Unlock）
- 序列号生成（Sequence）
- Session 管理
- Watch 通知机制
- 快照和恢复
- 多节点 Raft 集群

## 使用场景与最佳实践

- 分布式系统的元数据协调
- 消息队列的元数据后端
- 分布式锁和领导者选举
- 配置中心的底层存储
- ZooKeeper 的现代化替代

## 参考链接

- https://github.com/streamnative/oxia
- https://oxia.dev/

## Related

- [[17-系统基础/06-知识字典/fundamentals/etcd.md|etcd]]
- [[17-系统基础/06-知识字典/storage/tikv.md|TiKV]]
- [[17-系统基础/06-知识字典/storage/vineyard.md|Vineyard]]
