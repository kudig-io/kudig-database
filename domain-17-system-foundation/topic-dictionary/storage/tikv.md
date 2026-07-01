---
title: TiKV 分布式 KV 存储
description: TiKV 是 PingCAP 开源的 CNCF 毕业项目，分布式事务键值存储引擎，为 TiDB 提供底层存储，同时也可独立使用，支持强一致性和水平扩展。...
summary: TiKV 是 PingCAP 开源的 CNCF 毕业项目，分布式事务键值存储引擎，为 TiDB 提供底层存储，同时也可独立使用，支持强一致性和水平扩展。...
category: dictionary
tags:
- k8s
- glossary
- storage
- database
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
- TiKV 分布式 KV 存储 是什么
- TiKV 详解
trigger_keywords:
- TiKV 分布式 KV 存储
- TiKV
- dictionary
prerequisites:
- kubernetes
---



# TiKV 分布式 KV 存储（TiKV）

## 概述

TiKV 是 PingCAP 开源的 CNCF 毕业项目，分布式事务键值存储引擎，为 TiDB 提供底层存储，同时也可独立使用，支持强一致性和水平扩展。

## 核心概念/原理

- **分布式事务**：支持 ACID 事务（基于 Percolator 模型）
- **Raft 共识**：数据多副本强一致性
- **CNCF 毕业**：TiDB 生态的核心组件
- **水平扩展**：自动分片和负载均衡

## 关键机制或特性

- Multi-Raft Group 架构
- MVCC 多版本并发控制
- Coprocessor 下推计算
- Raw KV（无事务的低延迟访问）
- Titan（大 Value 优化存储引擎）
- PD（Placement Driver）元数据管理

## 使用场景与最佳实践

- TiDB 的分布式存储后端
- 需要强一致 KV 的微服务
- 元数据存储和管理
- 配置中心的底层存储
- 替代 etcd 的大规模 KV 场景

## 参考链接

- https://tikv.org/
- https://github.com/tikv/tikv

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/etcd.md|etcd]]
- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/vineyard.md|Vineyard]]
