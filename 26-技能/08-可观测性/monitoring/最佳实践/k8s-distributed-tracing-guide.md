---
title: Kubernetes 分布式追踪最佳实践
description: '# Kubernetes 分布式追踪最佳实践'
summary: '本指南提供生产环境 Kubernetes 分布式追踪配置的最佳实践，涵盖从 Jaeger 部署到 [[opentelemetry|OpenTelemetry]] 集成的全方位内容 ^[inferred]。'
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 分布式追踪最佳实践

## 概述

本指南提供生产环境 Kubernetes 分布式追踪配置的最佳实践，涵盖从 Jaeger 部署到 [[opentelemetry|OpenTelemetry]] 集成的全方位内容 ^[inferred]。

## 分布式追踪架构

采用五层架构 ^[inferred]：

- **应用层**：集成 OpenTelemetry SDK 的微服务
- **采集层**：OpenTelemetry Collector（统一采集）+ Jaeger Agent（Span 收集）
- **处理层**：采样器（概率/自适应）+ 处理器（数据转换）+ 导出器（多后端支持）
- **存储层**：Jaeger Collector -> Elasticsearch/Cassandra
- **可视化层**：Jaeger Query + Jaeger UI

## 关键配置

### OpenTelemetry Collector 配置

- 接收器：OTLP（[[grpc|gRPC]] 4317、HTTP 4318）+ Jaeger 协议（gRPC 14250、thrift_http 14268）^[inferred]
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

- [[22-概念/k8s-production-best-practices.md|[[22-概念/10-最佳实践/k8s-production-best-practices|Kubernetes 生产环境最佳实践]]]]
- [[22-概念/observability-pillars.md|[[22-概念/06-可观测性/observability-pillars|Observability Pillars]]]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-monitoring-guide.md|Kubernetes 监控最佳实践]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]]

## 生产案例

### 案例 1: Jaeger 采样率过高导致性能开销

| 时间 | 事件 |
|------|------|
| - | 应用 P99 延迟增加 50ms |
| - | 发现 Jaeger 采样率 100%，每个请求都上报 trace |
| - | 🟢 调整采样率为 1%(生产) + 错误请求 100% |

**根因**: 默认采样率 100% 适合开发，生产应大幅降低。

### 案例 2: Trace 数据丢失导致无法定位问题

**现象**: 部分请求在 Jaeger 中无 trace 数据。

**诊断**: OpenTelemetry Collector 队列满，丢弃数据

**修复**: 🟡 增加 Collector 副本 + 调整队列大小 + 启用持久化队列

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 追踪系统影响业务性能 | 降低采样率 |
| P1 | 数据丢失 | 扩容 Collector |
| P2 | 成本优化 | 调整保留策略 |

## 面试要点

1. **Q: 分布式追踪的核心概念？**
   A: Trace(一次完整请求链路) 由多个 Span(单个操作) 组成。每个 Span 包含: traceID、spanID、parentSpanID、操作名、时间戳、标签。通过 Context Propagation 跨服务传递。

2. **Q: OpenTelemetry 的架构？**
   A: SDK(应用内埋点) → Exporter → Collector(接收/处理/导出) → Backend(Jaeger/Zipkin/Tempo)。支持 OTLP、Jaeger、Zipkin 多种协议。

3. **Q: 生产环境采样策略？**
   A: ① 头部采样(Head-based): 入口处决定，简单但可能错过异常 ② 尾部采样(Tail-based): 收集全部后按规则保留，可捕获错误/慢请求 ③ 生产推荐: 1% 随机 + 100% 错误 + 100% 慢请求。

## Related

- [[jaeger]] — Jaeger
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/06-可观测性/observability-pillars.md|observability-pillars]] — Observability Pillars
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践


<!-- risk-assessed -->
