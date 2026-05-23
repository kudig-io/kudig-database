---
title: OpenTelemetry 与分布式链路追踪
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- grafana
- jaeger
- istio
- cilium
- opa
- daemonset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenTelemetry 与分布式链路追踪 是什么
- 如何 OpenTelemetry 与分布式链路追踪
trigger_keywords:
- OpenTelemetry
- 与分布式链路追踪
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

# [[OpenTelemetry|OpenTelemetry]] 与分布式链路追踪

## 概述

**OpenTelemetry（OTel）** 是 CNCF 毕业项目，已成为云原生可观测性领域的事实标准。它提供了统一的 API、SDK 和数据收集规范，用于采集**指标（Metrics）、日志（Logs）和链路追踪（Traces）**三类可观测性信号。2026 年，[[Kubernetes|Kubernetes]] 上的现代应用已普遍采用 OpenTelemetry 替代分散的 [[Prometheus|Prometheus]]、[[Jaeger|Jaeger]]、[[domain-19-landscape-references/01-cncf-landscape/graduated/fluentd/fluentd|[[Fluentd|Fluentd]]]] 等工具链，实现"一个 SDK、一个 Collector、多后端输出"的统一可观测架构。

## 核心概念/原理

### 1. 三大可观测性信号

OpenTelemetry 统一了三种核心信号：
- **Traces（链路追踪）**：记录请求在分布式系统中的完整调用链，帮助定位延迟瓶颈
- **Metrics（指标）**：聚合的数值测量，如请求速率、错误率、延迟分布
- **Logs（日志）**：离散的时间戳事件记录

OpenTelemetry 的核心理念是：**应用只需一次埋点，即可将数据发送到任意分析后端**（Prometheus、Grafana、Jaeger、Datadog、Splunk 等）。

### 2. OpenTelemetry Collector

**OTel Collector** 是一个与语言无关的代理，负责接收、处理和导出可观测性数据：
- **Receivers**：接收来自应用的数据（OTLP、Prometheus、Zipkin、Jaeger 等协议）
- **Processors**：对数据进行处理（批量化、过滤、采样、增强标签）
- **Exporters**：将数据发送到后端存储和分析系统
- **Pipelines**：将 Receivers → Processors → Exporters 组合成处理流水线

```yaml
# OpenTelemetry Collector 配置示例
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318
processors:
  batch:
exporters:
  prometheusremotewrite:
    endpoint: http://prometheus:9090/api/v1/write
  otlp/jaeger:
    endpoint: jaeger-collector:4317
service:
  pipelines:
    metrics:
      receivers: [otlp]
      processors: [batch]
      exporters: [prometheusremotewrite]
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [otlp/jaeger]
```

### 3. 自动埋点（Auto-instrumentation）

OpenTelemetry 支持多种语言的自动埋点，无需修改业务代码：
- **Java**：通过 Java Agent 自动拦截 Spring Boot、JDBC、gRPC 等框架调用
- **Python**：通过 `opentelemetry-instrument` CLI 自动注入
- **.NET / Node.js / Go**：均有相应的自动埋点库
- **Kubernetes Operator**：通过 Admission Webhook 自动为 Pod 注入 OTel Agent 和配置

### 4. Trace 核心概念

- **Trace**：一个完整请求的树状调用链
- **Span**：Trace 中的单个操作单元，包含开始时间、持续时间、操作名、标签和事件
- **Context Propagation**：通过 HTTP Header（如 `traceparent`）将 Trace ID 跨服务传递
- **Baggage**：随 Trace 上下文传递的键值对，可用于业务标签透传

## 关键机制或特性

### 采样策略（Sampling）

高流量系统中无法采集 100% 的 Trace，需要通过采样控制数据量：
- **Head-based Sampling**：在请求入口处决定是否采样，实现简单但可能遗漏异常路径
- **Tail-based Sampling**：收集完整 Trace 后再根据规则（如包含错误 Span）决定是否保留，更准确但内存开销更大

### 与 Service Mesh 集成

Istio、Linkerd、Cilium Service Mesh 均可自动生成分布式追踪数据并导出为 OTLP 格式：
- 服务网格生成 L4/L7 流量的 Span
- 应用通过 OpenTelemetry SDK 生成业务逻辑 Span
- 两者通过统一的 Trace ID 拼接成完整的调用链路

### Kubernetes 上的部署模式

| 模式 | 架构 | 适用场景 |
|------|------|----------|
| **DaemonSet Collector** | 每个节点一个 Collector | 大规模集群，减少应用端配置 |
| **Sidecar Collector** | 每个 Pod 一个 Collector | 多租户隔离、独立配置需求 |
| **Deployment Collector** | 集群级共享 Collector | 中小规模、简化部署 |

## 使用场景

1. **微服务故障定位**：用户请求响应慢，通过 Trace 快速定位是数据库查询、缓存未命中还是下游服务超时
2. **AI Pipeline 可观测性**：追踪从数据预处理、模型推理到后处理的完整链路，优化端到端延迟
3. **性能回归分析**：对比发布前后的 P99 延迟 Trace，定位新版本引入的性能退化点
4. **多语言系统统一观测**：Java 后端、Python 推理服务、Go Gateway 统一接入 OpenTelemetry，实现跨语言 Trace 串联
5. **成本优化**：通过 Tail-based Sampling 保留异常 Trace，同时丢弃正常的海量请求 Trace，降低存储成本

## 最佳实践/注意事项

- **Collector 高可用**：生产环境必须部署多副本 Collector，并配置负载均衡和健康检查
- **使用 OTLP 协议**：优先使用 OpenTelemetry 原生的 OTLP/gRPC 协议，比 Zipkin/Jaeger 专有协议性能更好
- **资源标签标准化**：统一使用 `service.name`、`k8s.namespace.name`、`deployment.environment` 等语义约定标签
- **合理配置采样率**：开发环境可 100% 采样，生产环境根据流量规模设置 1%–10% 的 Head-based 采样
- **避免 Baggage 滥用**：Baggage 会随每个请求跨服务传递，数据量过大将增加网络开销
- **关注 Collector 自身性能**：在高吞吐场景下，Collector 可能成为瓶颈，需监控其 CPU、内存和队列深度
- **Secret 保护**：Collector 配置中可能包含后端 API Key，应使用 Kubernetes Secret 或 External Secrets 管理
- **Trace 与 Log 关联**：在日志中注入 Trace ID，实现从指标告警 → Trace → 日志的完整可观测性跳转

## 参考链接

- [OpenTelemetry Documentation](https://opentelemetry.io/docs/)
- [OpenTelemetry Collector](https://opentelemetry.io/docs/collector/)
- [OpenTelemetry Kubernetes Operator](https://opentelemetry.io/docs/kubernetes/operator/)
- [Jaeger Documentation](https://www.jaegertracing.io/docs/)
- [Wiz.io - AI/ML in Kubernetes Best Practices](https://www.wiz.io/academy/ai-security/ai-ml-kubernetes-best-practices)

## Related

- [[domain-19-landscape-references/topic-index/service-mesh-index|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
