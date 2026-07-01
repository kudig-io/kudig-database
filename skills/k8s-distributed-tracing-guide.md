---
title: Kubernetes 分布式追踪最佳实践
description: '# Kubernetes 分布式追踪最佳实践'
category: skills
tags:
- k8s
- tracing
- jaeger
- opentelemetry
- distributed-tracing
- helm
- elasticsearch
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 分布式追踪最佳实践 是什么
- 如何 Kubernetes 分布式追踪最佳实践
trigger_keywords:
- Kubernetes
- 分布式追踪最佳实践
prerequisites:
- kubectl-basics
- helm-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

# Kubernetes 分布式追踪最佳实践

## 概述

本指南提供生产环境 Kubernetes 分布式追踪配置的最佳实践，涵盖从 Jaeger 部署到 [[OpenTelemetry|OpenTelemetry]] 集成的全方位内容 ^[inferred]。

## 分布式追踪架构

采用五层架构 ^[inferred]：

- **应用层**：集成 OpenTelemetry SDK 的微服务
- **采集层**：OpenTelemetry Collector（统一采集）+ Jaeger Agent（Span 收集）
- **处理层**：采样器（概率/自适应）+ 处理器（数据转换）+ 导出器（多后端支持）
- **存储层**：Jaeger Collector -> Elasticsearch/Cassandra
- **可视化层**：Jaeger Query + Jaeger UI

## 关键配置

### OpenTelemetry Collector 配置

- 接收器：OTLP（[[gRPC|gRPC]] 4317、HTTP 4318）+ Jaeger 协议（gRPC 14250、thrift_http 14268）^[inferred]
- 处理器：`batch`（timeout: 5s, send_batch_size: 1024）+ `memory_limiter`（limit_mib: 512）^[inferred]
- 导出器：OTLP 到 Jaeger Collector ^[inferred]

### Jaeger 生产配置

- `strategy: production` — 生产模式部署 ^[inferred]
- Collector 副本数最大 5，资源 requests: 512Mi/500m，limits: 1Gi/1CPU ^[inferred]
- 存储后端推荐 Elasticsearch，配置 index-prefix、num-shards: 3、num-replicas: 1 ^[inferred]

### 采样策略

- 推荐 `parentbased_traceidratio` 采样器，采样率 0.1（10%）^[inferred]
- 过高采样率会导致性能开销和存储成本增加 ^[inferred]
- 生产环境建议使用自适应采样 ^[ambiguous]

### 追踪上下文传播

- 使用 W3C 标准：`tracecontext,baggage` ^[inferred]
- 跨服务追踪断裂通常由上下文传播配置不当引起 ^[inferred]

## 实施步骤

1. **安装 Jaeger Operator**：通过 Helm 安装
2. **部署 Jaeger 实例**：production 模式，配置 Elasticsearch 存储
3. **安装 OpenTelemetry Collector**：作为统一采集层
4. **配置应用集成**：通过环境变量配置 OTEL SDK

## 常见陷阱

### 采样率配置不当

采样率过高导致性能开销和存储成本增加。建议 10% 采样率，或使用自适应采样 ^[inferred]。

### 追踪上下文传播失败

跨服务追踪上下文传播失败会导致分布式追踪不完整。应使用 W3C TraceContext 标准传播器 ^[inferred]。

### 存储后端配置不当

存储后端配置不当会导致数据丢失。应配置 Elasticsearch 分片和副本，确保高可用 ^[inferred]。

## 验证方法

- 检查 Jaeger 状态：`kubectl get jaeger -n tracing`
- 检查 OpenTelemetry Collector 状态
- 测试追踪数据：`curl http://localhost:16686/api/services`

## 相关资源

- [[concepts/k8s-production-best-practices.md|[[Kubernetes 生产环境最佳实践|Kubernetes 生产环境最佳实践]]]]
- [[concepts/observability-pillars.md|[[Observability Pillars|Observability Pillars]]]]
- [[skills/k8s-monitoring-guide.md|Kubernetes 监控最佳实践]]
- [[skills/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]]

## Related

- [[jaeger]] — Jaeger
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/observability-pillars.md|observability-pillars]] — Observability Pillars
- [[concepts/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
