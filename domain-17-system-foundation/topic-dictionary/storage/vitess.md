---
title: Vitess MySQL 分片
description: 'Vitess 是 PlanetScale 开源的 CNCF 毕业项目，为 MySQL 提供水平扩展和分片能力，通过透明分片让应用无需修改即可扩展到多个 MySQ...'
category: dictionary
tags:
- k8s
- glossary
- storage
- database
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Vitess MySQL 分片 是什么
- Vitess 详解
trigger_keywords:
- Vitess MySQL 分片
- Vitess
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Vitess MySQL 分片（Vitess）

## 概述

Vitess 是 PlanetScale 开源的 CNCF 毕业项目，为 MySQL 提供水平扩展和分片能力，通过透明分片让应用无需修改即可扩展到多个 MySQL 实例，是 YouTube 的数据库基础设施。

## 核心概念/原理

- **MySQL 兼容**：100% 兼容 MySQL 协议，应用无需修改
- **透明分片**：自动将查询路由到正确的分片
- **CNCF 毕业**：YouTube/GitHub/Slack 等使用
- **在线迁移**：支持在线分片和数据迁移

## 关键机制或特性

- VTGate（查询路由代理）
- VTTablet（分片管理代理）
- VSchema（分片规则定义）
- MoveTables/Reshard（在线数据迁移）
- 连接池和查询缓存
- 自动故障转移和备份恢复

## 使用场景与最佳实践

- MySQL 数据库的水平扩展
- 从单库到分片的在线迁移
- 大规模 MySQL 集群管理
- 需要 MySQL 兼容性的云原生数据库
- 多租户数据库的分片隔离

## 参考链接

- https://vitess.io/
- https://github.com/vitessio/vitess

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/tikv.md|TiKV]]
- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg.md|CloudNativePG]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes.md|PV/PVC]]
