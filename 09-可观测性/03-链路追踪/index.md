---
title: Distributed Tracing
description: 链路追踪知识域 — OpenTelemetry 全链路、Jaeger/Tempo 部署、采样策略、Span 关联分析
category: subdomain
tags:
- tracing
- opentelemetry
- jaeger
- tempo
- distributed-tracing
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 分布式链路追踪 Distributed Tracing

> 端到端请求可视化，快速定位微服务架构下的性能瓶颈与故障根因。

## 追踪后端对比

| 工具 | 存储 | 优势 | 适用场景 |
|------|------|------|----------|
| Jaeger | ES/Cassandra/Kafka | CNCF 毕业、生态成熟 | 通用微服务追踪 |
| Grafana Tempo | 对象存储(S3/GCS) | 低成本、与 Grafana 深度集成 | 已有 Grafana 栈 |
| Zipkin | MySQL/ES | 轻量、快速上手 | 小规模/开发环境 |
| SigNoz | ClickHouse | 全合一（Metrics+Traces+Logs） | 初创团队 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[09-可观测性/04-链路追踪/01-jaeger-production-deployment.md\|Jaeger 生产部署]] | 架构/存储/采样/运维 | advanced |
| [[09-可观测性/04-链路追踪/02-grafana-tempo-tracing.md\|Grafana Tempo 追踪]] | 对象存储后端、TraceQL | intermediate |
| [[09-可观测性/04-链路追踪/03-opentelemetry-collector-patterns.md\|OTel Collector 模式]] | 采集器部署拓扑与管道配置 | advanced |
| [[09-可观测性/03-链路追踪/04-opentelemetry-distributed-tracing.md\|OTel 分布式追踪]] | SDK 集成、Context 传播 | intermediate |
| [[09-可观测性/03-链路追踪/05-distributed-tracing.md\|分布式追踪原理]] | Trace/Span/Baggage 核心概念 | beginner |
| [[09-可观测性/03-链路追踪/08-distributed-tracing-guide.md\|追踪实践指南]] | 生产环境最佳实践汇总 | advanced |

## 采样策略选择

| 策略 | 原理 | 适用 |
|------|------|------|
| Head-based | 入口决定采样率 | 低流量/均匀采样 |
| Tail-based | 完整链路后决策 | 只保留异常/慢请求 |
| 自适应 | 根据流量动态调整 | 高流量生产环境 |

## Related

- [[09-可观测性/02-指标/index.md|指标 Metrics]]
- [[09-可观测性/07-工具/index.md|可观测性工具]]
- [[09-可观测性/06-SLO-SLI/index.md|SLO & SLI]]
