---
title: Prometheus TSDB 深度解析
description: '# Prometheus TSDB 深度解析'
summary: '2h Block (内存) → Compaction → 8h Block → Compaction → 2d Block'
category: domain
tags:
- prometheus
- tsdb
- time-series
- storage
- observability
- flux
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Prometheus TSDB 深度解析 是什么
- 如何 Prometheus TSDB 深度解析
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- Prometheus
- TSDB
- 深度解析
- database
- middleware
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Prometheus|Prometheus]] TSDB 深度解析

## 存储架构

```
TSDB 存储结构:
├── data/
│   ├── wal/              # Write Ahead Log
│   ├── chunks_head/      # 内存中的 chunks（mmap）
│   ├── 01B.../           # 已持久化的 block
│   │   ├── chunks/       # 压缩后的样本数据
│   │   ├── index         # 倒排索引
│   │   ├── meta.json     # 元数据
│   │   └── tombstones    # 删除标记
│   └── chunks_head/
└── queries.active        # 活跃查询记录
```

## Block 生命周期

```
2h Block (内存) → Compaction → 8h Block → Compaction → 2d Block
     ↑              ↓              ↓              ↓
   Head Block    压缩合并      进一步压缩      长期保留
```

## 高基数问题

```
问题: 过多的唯一时间序列导致内存和查询性能下降

原因:
  - 未限制的 label 值（如 user_id、request_id）
  - 过多的 metric 名称

解决:
  - 使用 recording rules 预聚合
  - 限制高基数 label
  - 使用 remote write 分流
  - 启用 compaction
```

## 相关

- observability/02-metrics/02-monitoring-metrics-system]]
- [[domain-16-database-middleware/时序数据库/02-influxdb-vs-timescaledb.md|02 influxdb vs timescaledb]]


<!-- risk-assessed -->
