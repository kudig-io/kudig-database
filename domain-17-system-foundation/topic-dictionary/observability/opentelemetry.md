---
title: OpenTelemetry
description: OpenTelemetry（简称 OTel）是 CNCF 孵化项目，提供统一的分布式系统可观测性标准。它将 Traces、Metrics、Logs
  三大支柱统一...
summary: OpenTelemetry（简称 OTel）是 CNCF 孵化项目，提供统一的分布式系统可观测性标准。它将 Traces、Metrics、Logs
  三大支柱统一...
category: dictionary
tags:
- k8s
- glossary
- opentelemetry
- observability
- tracing
- metrics
- logging
tier: core
created: 2026-05
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenTelemetry 是什么
- OpenTelemetry (OTel) 详解
trigger_keywords:
- OpenTelemetry
- OpenTelemetry (OTel)
- dictionary
prerequisites:
- kubectl-basics
---



# OpenTelemetry

> **英文名**: OpenTelemetry (OTel)

## 概述

OpenTelemetry（简称 OTel）是 CNCF 孵化项目，提供统一的分布式系统可观测性标准。它将 Traces、Metrics、Logs 三大支柱统一在一套 API 和 SDK 中，已成为可观测性数据采集的事实标准。

## 核心概念/原理

### 三大支柱

| 支柱 | 说明 | 代表工具 |
|------|------|----------|
| Traces | 请求在分布式系统中的完整路径 | Jaeger、Tempo |
| Metrics | 系统/应用的数值指标 | Prometheus、VictoriaMetrics |
| Logs | 结构化日志事件 | Loki、ELK |

### 核心组件

- **API**：语言无关的观测数据生成接口。
- **SDK**：API 的实现，包含采样、处理和导出。
- **OTLP**：OpenTelemetry Protocol，统一的数据传输协议。
- **Collector**：数据收集、处理和路由的中间层。

## 关键机制或特性

- **OTLP 协议**：基于 gRPC/HTTP，统一传输 Traces、Metrics、Logs。
- **自动 Instrumentation**：Java Agent、Node.js SDK 等无需修改代码即可采集。
- **Collector Pipeline**：receivers → processors → exporters，灵活路由数据。
- **Context Propagation**：通过 W3C Trace Context 在微服务间传递追踪上下文。
- 支持 Kubernetes Operator 自动注入 Instrumentation。

## 使用场景与最佳实践

- 新项目直接使用 OpenTelemetry SDK 替代 Jaeger/Zipkin 等独立方案。
- 部署 OTel Collector 统一采集和路由观测数据。
- 使用 auto-instrumentation 降低接入成本。
- 配置合理的采样率避免数据爆炸（如 traceID ratio 采样）。
- 结合 Grafana Tempo/Jaeger 可视化和查询追踪数据。

## 参考链接

- [OpenTelemetry Official](https://opentelemetry.io/docs/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/jaeger.md|Jaeger]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging.md|Logging]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]
