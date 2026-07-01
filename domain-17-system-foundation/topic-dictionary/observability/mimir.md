---
title: Mimir
description: Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex
  的下一代替代品，提供水...
summary: Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex 的下一代替代品，提供水...
category: dictionary
tags:
- k8s
- glossary
- mimir
- prometheus
- observability
- grafana
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Mimir 是什么
- Mimir 详解
trigger_keywords:
- Mimir
- dictionary
prerequisites:
- kubectl-basics
---



# Mimir

> **英文名**: Mimir

## 概述

Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex 的下一代替代品，提供水平扩展、多租户和长期指标存储能力。

## 核心概念/原理

### 与 Cortex/Thanos 对比

| 特性 | Mimir | Cortex | Thanos |
|------|-------|--------|--------|
| 状态 | 活跃开发 | 维护模式 | 活跃 |
| 架构 | 单体微服务混合 | 纯微服务 | Sidecar |
| 查询 | PromQL 兼容 | PromQL 兼容 | PromQL 兼容 |
| 多租户 | 原生 | 原生 | 需额外 |

### 核心组件

Distributor、Ingester、Querier、Query-Frontend、Compactor、Store-Gateway、Ruler。

## 关键机制或特性

- **水平扩展**：每个组件可独立扩缩容。
- **PromQL 兼容**：完全兼容 Prometheus 查询语言。
- **Ruler**：分布式规则评估和告警。
- **对象存储**：TSDB 数据存储在 S3/GCS/MinIO。
- 支持 Remote Write 接收指标数据。

## 使用场景与最佳实践

- 大规模 Prometheus 部署使用 Mimir 替代 Thanos。
- 多租户环境使用 Mimir 的租户隔离功能。
- 配合 Grafana 构建统一的指标可视化。
- 使用 Remote Write 将多个 Prometheus 实例的数据汇聚到 Mimir。
- 配置 Compactor 的保留策略管理存储成本。

## 参考链接

- [Mimir Official](https://grafana.com/oss/mimir/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/thanos.md|Thanos]]
- [[domain-17-system-foundation/topic-dictionary/observability/promql.md|PromQL]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]
