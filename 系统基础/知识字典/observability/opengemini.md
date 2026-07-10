---
title: openGemini 时序数据库
description: openGemini 是华为开源的 CNCF Sandbox 时序数据库，兼容 InfluxDB 协议，专为 IoT 和可观测性场景优化，提供高写入吞吐和低存储...
summary: openGemini 是华为开源的 CNCF Sandbox 时序数据库，兼容 InfluxDB 协议，专为 IoT 和可观测性场景优化，提供高写入吞吐和低存储...
category: dictionary
tags:
- k8s
- glossary
- observability
- database
- tsdb
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- openGemini 时序数据库 是什么
- openGemini 详解
trigger_keywords:
- openGemini 时序数据库
- openGemini
- dictionary
prerequisites:
- kubernetes
---



# openGemini 时序数据库（openGemini）

## 概述

openGemini 是华为开源的 CNCF Sandbox 时序数据库，兼容 InfluxDB 协议，专为 IoT 和可观测性场景优化，提供高写入吞吐和低存储成本。

## 核心概念/原理

- **InfluxDB 兼容**：兼容 InfluxDB 查询语言和 API
- **高吞吐**：百万级数据点/秒写入
- **CNCF Sandbox**：华为主导
- **云原生**：存算分离架构

## 关键机制或特性

- SQL-like 查询语言（类 InfluxQL）
- 存算分离（支持 S3/HDFS 存储后端）
- 时序数据自动压缩和降采样
- 集群模式和 HA
- 内置数据分区和保留策略
- Prometheus Remote Write 接收

## 使用场景与最佳实践

- IoT 设备指标的时序存储
- 可观测性数据的长期存储
- InfluxDB 的国产替代方案
- 大规模时序数据的高性能查询
- 与 Prometheus/Grafana 集成的监控栈

## 参考链接

- https://opengemini.github.io/
- https://github.com/openGemini/openGemini

## Related

- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]
- [[系统基础/知识字典/observability/thanos.md|Thanos]]
- [[系统基础/知识字典/observability/mimir.md|Mimir]]
