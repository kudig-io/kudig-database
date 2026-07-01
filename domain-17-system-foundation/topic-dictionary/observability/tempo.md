---
title: Tempo
description: 'Grafana Tempo 是 Grafana Labs 开源的大规模分布式追踪后端，专为对象存储设计。它以低成本存储追踪数据，与 Grafana 和 Loki...'
category: dictionary
tags:
- k8s
- glossary
- tempo
- tracing
- observability
- grafana
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tempo 是什么
- Tempo 详解
trigger_keywords:
- Tempo
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Tempo

> **英文名**: Tempo

## 概述

Grafana Tempo 是 Grafana Labs 开源的大规模分布式追踪后端，专为对象存储设计。它以低成本存储追踪数据，与 Grafana 和 Loki 深度集成，是 Prometheus + Loki + Tempo 可观测性三件套的追踪组件。

## 核心概念/原理

### 核心架构

| 组件 | 功能 |
|------|------|
| Distributor | 接收和分发 span 数据 |
| Ingester | 缓冲并写入对象存储 |
| Compactor | 合并和压缩 trace 块 |
| Querier | 按 TraceID 查询 |
| Query-Frontend | 查询加速和缓存 |

### 设计理念

- **仅按 TraceID 索引**：不做全文索引，极大降低存储成本。
- **对象存储原生**：数据存储在 S3/GCS/MinIO。
- **与 Grafana 集成**：在 Grafana 中联合查询 Metrics + Logs + Traces。

## 关键机制或特性

- **OTLP 原生**：直接接收 OpenTelemetry 协议数据。
- **低成本**：存储成本比 Jaeger（ES 后端）低 5-10 倍。
- **Metrics-from-Traces**：从 span 数据自动生成指标。
- **TraceQL**：结构化查询语言（类似 LogQL）。
- 支持多租户隔离。

## 使用场景与最佳实践

- Grafana 生态用户选择 Tempo 替代 Jaeger 存储追踪数据。
- 配合 OpenTelemetry Collector 采集和路由 span 数据。
- 在 Grafana 中实现 Metrics → Logs → Traces 的关联查询。
- 使用 TraceQL 进行高级追踪数据查询。
- 配置合理的采样率控制存储成本。

## 参考链接

- [Tempo Official](https://grafana.com/oss/tempo/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/jaeger.md|Jaeger]]
- [[domain-17-system-foundation/topic-dictionary/observability/loki.md|Loki]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
