---
title: Jaeger
description: Jaeger 是 CNCF 毕业项目，提供分布式追踪（Distributed Tracing）功能，用于监控和排查微服务架构中的请求链路。它兼容
  OpenTel...
summary: Jaeger 是 CNCF 毕业项目，提供分布式追踪（Distributed Tracing）功能，用于监控和排查微服务架构中的请求链路。它兼容
  OpenTel...
category: dictionary
tags:
- k8s
- glossary
- jaeger
- tracing
- observability
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Jaeger 是什么
- Jaeger 详解
trigger_keywords:
- Jaeger
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Jaeger

> **英文名**: Jaeger

## 概述

Jaeger 是 CNCF 毕业项目，提供分布式追踪（Distributed Tracing）功能，用于监控和排查微服务架构中的请求链路。它兼容 OpenTelemetry，是 Kubernetes 生态中最常用的追踪后端之一。

## 核心概念/原理

### 核心架构

- **Jaeger Agent**：轻量级 sidecar，接收应用发送的 span 数据。
- **Jaeger Collector**：接收和处理 span，写入存储后端。
- **Jaeger Query**：提供 Web UI 和 API 查询追踪数据。
- **Storage**：支持 Elasticsearch、Cassandra、Kafka + Flink 等。

### 追踪模型

```
Trace
├── Span A (Service A)
│   ├── Span B (Service B)
│   │   └── Span D (Database)
│   └── Span C (Service C)
```

## 关键机制或特性

- 完全兼容 OpenTelemetry Collector 和 OTLP 协议。
- 支持自适应采样（Adaptive Sampling），根据流量自动调整。
- Jaeger v2 基于 OpenTelemetry Collector 架构重构。
- 支持 Service Performance Monitoring（SPM）自动聚合指标。
- 提供 Spark/Flink 作业进行离线数据分析。

## 使用场景与最佳实践

- 新部署推荐使用 Jaeger v2（基于 OTel Collector）。
- 配合 OpenTelemetry SDK 采集追踪数据。
- 使用 Jaeger UI 分析慢请求和错误链路。
- 为高流量服务配置合理采样率（如 1%）。
- 存储后端优先选择 Elasticsearch 或 Tempo。

## 参考链接

- [Jaeger Official](https://www.jaegertracing.io/)

## Related

- [[17-系统基础/06-知识字典/observability/opentelemetry.md|OpenTelemetry]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]
- [[17-系统基础/06-知识字典/observability/grafana.md|Grafana]]
- [[17-系统基础/06-知识字典/observability/logging.md|Logging]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]


<!-- risk-assessed -->
- [[01-集群基础/02-设计原则/17-observability-design-principles|16 - 可观测性设计原则]]
- [[01-集群基础/02-设计原则/15-service-mesh-architecture|14 - 服务网格与微服务架构设计]]
- [[04-应用模式/02-行业架构/01-ecommerce-architecture|电商系统 Kubernetes 生产架构设计 (应用模式)]]
- [[05-网络/04-API网关/12-api-gateway-observability|12 - API 网关可观测性：指标、日志与链路追踪]]
- [[10-平台工程/04-开发体验/04-platform-team-topology|平台团队拓扑与运营 (Platform Team Topology and Operations)]]
- [[16-专项技术/02-WebAssembly/04-wasmcloud-platform|wasmCloud 平台]]
- [[17-系统基础/06-知识字典/observability/opentelemetry-and-distributed-tracing|OpenTelemetry 与分布式链路追踪]]
- [[17-系统基础/06-知识字典/operations/enterprise-ops-practices|企业级运维最佳实践]]
- [[22-概念/11-交叉分析/可观测性支柱 × Prometheus-Grafana|可观测性支柱 × Prometheus-Grafana]]
- [[23-实体/15-参考与索引/observability-terms|K8s 可观测性术语参考]]
- [[23-实体/15-参考与索引/tooling-terms|K8s 工具链术语参考]]
- [[26-技能/04-工作负载/pod/诊断排障/structural-01-pod-troubleshooting|Pod 故障排查与运行机制深度指南 [topic-structural-trouble-shooting]]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]]
