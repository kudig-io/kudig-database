---
title: InfluxDB vs TimescaleDB 对比
description: '## 架构差异'
category: domain
tags:
- influxdb
- timescaledb
- time-series
- postgresql
- comparison
- prometheus
- flux
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- InfluxDB vs TimescaleDB 对比 是什么
- 如何 InfluxDB vs TimescaleDB 对比
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- InfluxDB
- vs
- TimescaleDB
- 对比
- database
- middleware
prerequisites:
- kubectl-basics
- prometheus-basics
created: "2026-05-23"
---

# InfluxDB vs TimescaleDB 对比

## 架构差异

| 维度 | InfluxDB v2 | TimescaleDB |
|------|-------------|-------------|
| 基础 | 专用存储引擎 | PostgreSQL 扩展 |
| 查询语言 | [[Flux|Flux]] / InfluxQL | SQL + 时序扩展 |
| 生态 | Influx 生态 | PostgreSQL 生态 |
| 高可用 | Enterprise / Cloud | Patroni / 流复制 |
| 压缩率 | 高 | 中 |
| 学习曲线 | 需学 Flux | 标准 SQL |

## 选型建议

```
选择 InfluxDB:
  - 纯时序场景，无关系数据需求
  - 需要极高的写入吞吐
  - 使用 TICK 栈

选择 TimescaleDB:
  - 已有 PostgreSQL 基础设施
  需要 SQL 和关系查询能力
  - 团队熟悉 SQL
```

## 相关

- [[domain-16-database-middleware/04-time-series-db/01-prometheus-tsdb-deep-dive.md|01 prometheus tsdb deep dive]]
