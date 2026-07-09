---
title: K8s 可观测性术语参考
description: '| **告警与 SLO 监控工程** | Alerting And Slo Monitoring | 有效的告警系统不仅仅是"出问题时通知人"，而是要在**用户受到影响之前**准确捕捉异常信号，同时避免告警疲劳
  |'
summary: '| **告警与 SLO 监控工程** | Alerting And Slo Monitoring | 有效的告警系统不仅仅是"出问题时通知人"，而是要在**用户受到影响之前**准确捕捉异常信号，同时避免告警疲劳
  |'
category: references
tags:
- k8s
- dictionary
- observability
- prometheus
- grafana
- jaeger
- elasticsearch
- llm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 可观测性术语参考 是什么
- 如何 K8s 可观测性术语参考
trigger_keywords:
- K8s
- 可观测性术语参考
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 可观测性术语参考

本页汇总了 **可观测性** 领域的 10 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[entities/k8s-observability-ecosystem.md|k8s-observability-ecosystem]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **告警与 SLO 监控工程** | Alerting And Slo Monitoring | 有效的告警系统不仅仅是"出问题时通知人"，而是要在**用户受到影响之前**准确捕捉异常信号，同时避免告警疲劳 |
| **LLM 可观测性** | Llm Observability | 大语言模型（LLM）的可观测性远不止传统的 CPU、内存和延迟监控 |
| **日志聚合与 Loki** | Log Aggregation With Loki | 在 Kubernetes 环境中，日志分散在数百甚至数万个 Pod 中，**日志聚合（Log Aggregation）** 是运维排障和审计合规的基石 |
| **日志架构（Logging Architecture）** | Logging Architecture | 应用日志是理解集群内部运行情况、调试问题和监控集群活动的重要手段 |
| **Kubernetes 对象状态指标（kube-state-metrics）** | Metrics For Kubernetes Object States | kube-state-metrics 是一个 Kubernetes 插件代理，用于从 Kubernetes API 中对象的状态生成并暴露集群级指标 |
| **Kubernetes 系统组件指标** | Metrics For Kubernetes System Components | Kubernetes 系统组件指标能够帮助我们深入了解集群内部的运行状况，对于构建监控仪表板和告警系统尤为重要 |
| **可观测性（Observability）** | Observability | 在 Kubernetes 中，可观测性是通过收集和分析**指标（Metrics）**、**日志（Logs）**和**链路追踪（Traces）**——即可观... |
| **OpenTelemetry 与分布式链路追踪** | Opentelemetry And Distributed Tracing | **OpenTelemetry（OTel）** 是 CNCF 毕业项目，已成为云原生可观测性领域的事实标准 |
| **系统日志（System Logs）** | System Logs | 系统组件日志记录了集群中发生的事件，对于调试和故障排查非常有用 |
| **Kubernetes 系统组件链路追踪** | Traces For Kubernetes System Components | FEATURE STATE: `Kubernetes v1 |

---

### 告警与 SLO 监控工程

有效的告警系统不仅仅是"出问题时通知人"，而是要在**用户受到影响之前**准确捕捉异常信号，同时避免告警疲劳。2026 年的最佳实践将 **SLO（Service Level Objective）** 作为告警设计的核心锚点，通过 **Multi-window Multi-burn-rate** 策略实现高信噪比的告警体系。Prometheus + Alertmanager 仍是 Kubernetes 环境的主流组合，但越来越多的组织开始引入 **Cortex、Thanos、VictoriaMetrics** 来解决大规模集群的指标存储和全局查询问题。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/alerting-and-slo-monitoring.md`）*

---

### LLM 可观测性

大语言模型（LLM）的可观测性远不止传统的 CPU、内存和延迟监控。2026 年的 AI 生产系统需要追踪**提示词（Prompt）、响应（Response）、Token 消耗、模型输出质量、幻觉率（Hallucination Rate）以及成本**等专属指标。与常规微服务不同，LLM 推理的"正确性"往往是主观的，因此可观测性必须结合**自动化评估、人类反馈回路和 A/B 测试对比**来实现持续优化。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/llm-observability.md`）*

---

### 日志聚合与 Loki

在 Kubernetes 环境中，日志分散在数百甚至数万个 Pod 中，**日志聚合（Log Aggregation）** 是运维排障和审计合规的基石。2026 年的主流方案是 **Grafana Loki** —— 一个受 Prometheus 启发的水平可扩展日志聚合系统。与传统方案（如 Elasticsearch）相比，Loki 只索引日志的**标签（Labels）**而不索引日志内容本身，这使其在存储成本和运维复杂度上具有显著优势，特别适合 Kubernetes 的云原生场景。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/log-aggregation-with-loki.md`）*

---

### 日志架构（Logging Architecture）

应用日志是理解集群内部运行情况、调试问题和监控集群活动的重要手段。容器化应用最常见的日志记录方式是写入标准输出（`stdout`）和标准错误（`stderr`）。然而，仅靠容器引擎的原生功能通常不足以构建完整的日志解决方案。Kubernetes 引入了**集群级日志（cluster-level logging）**的概念，要求日志拥有独立于节点、Pod 和容器的存储和生命周期。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/logging-architecture.md`）*

---

### Kubernetes 对象状态指标（kube-state-metrics）

kube-state-metrics 是一个 Kubernetes 插件代理，用于从 Kubernetes API 中对象的状态生成并暴露集群级指标。它连接到 API 服务器，通过 HTTP 端点暴露由集群中各个对象状态生成的指标，使运维人员能够基于对象状态进行查询和告警。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/metrics-for-kubernetes-object-states.md`）*

---

### Kubernetes 系统组件指标

Kubernetes 系统组件指标能够帮助我们深入了解集群内部的运行状况，对于构建监控仪表板和告警系统尤为重要。Kubernetes 组件以 Prometheus 文本格式暴露指标，便于人和机器共同阅读和处理。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/metrics-for-kubernetes-system-components.md`）*

---

### 可观测性（Observability）

在 Kubernetes 中，可观测性是通过收集和分析**指标（Metrics）**、**日志（Logs）**和**链路追踪（Traces）**——即可观测性的三大支柱——来更好地理解集群的内部状态、性能和健康情况的过程。控制平面组件和许多插件都会生成这些信号，通过聚合和关联它们，可以获得跨集群的统一视图。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/observability.md`）*

---

### OpenTelemetry 与分布式链路追踪

**OpenTelemetry（OTel）** 是 CNCF 毕业项目，已成为云原生可观测性领域的事实标准。它提供了统一的 API、SDK 和数据收集规范，用于采集**指标（Metrics）、日志（Logs）和链路追踪（Traces）**三类可观测性信号。2026 年，Kubernetes 上的现代应用已普遍采用 OpenTelemetry 替代分散的 Prometheus、Jaeger、Fluentd 等工具链，实现"一个 SDK、一个 Collector、多后端输出"的统一可观测架构。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/opentelemetry-and-distributed-tracing.md`）*

---

### 系统日志（System Logs）

系统组件日志记录了集群中发生的事件，对于调试和故障排查非常有用。通过配置日志详细程度（verbosity），可以查看从粗粒度的错误信息到细粒度的逐步事件跟踪（如 HTTP 访问日志、Pod 状态变化、控制器操作、调度器决策等）等不同级别的日志内容。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/system-logs.md`）*

---

### Kubernetes 系统组件链路追踪

FEATURE STATE: `Kubernetes v1.27 [beta]`

系统组件链路追踪记录了集群中各操作之间的延迟和关系。Kubernetes 组件通过 **OpenTelemetry Protocol (OTLP)** 使用 gRPC exporter 发出追踪数据（trace spans），这些数据可以通过 OpenTelemetry Collector 收集并路由到追踪后端，用于可视化端到端请求流、诊断性能问题和识别瓶颈。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/observability/traces-for-kubernetes-system-components.md`）*

---

## 相关页面

- [[entities/k8s-observability-ecosystem.md|k8s-observability-ecosystem]]

## 来源文件

- `系统基础/topic-dictionary/observability/alerting-and-slo-monitoring.md`
- `系统基础/topic-dictionary/observability/llm-observability.md`
- `系统基础/topic-dictionary/observability/log-aggregation-with-loki.md`
- `系统基础/topic-dictionary/observability/logging-architecture.md`
- `系统基础/topic-dictionary/observability/metrics-for-kubernetes-object-states.md`
- `系统基础/topic-dictionary/observability/metrics-for-kubernetes-system-components.md`
- `系统基础/topic-dictionary/observability/observability.md`
- `系统基础/topic-dictionary/observability/opentelemetry-and-distributed-tracing.md`
- `系统基础/topic-dictionary/observability/system-logs.md`
- `系统基础/topic-dictionary/observability/traces-for-kubernetes-system-components.md`

## Related

- [[grpc]] — gRPC
- [[cortex]] — Cortex
- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
