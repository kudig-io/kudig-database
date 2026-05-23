---
title: 可观测性支柱 × Prometheus-Grafana
description: '[[concepts/observability-pillars]] 描述 Metrics/Logs/Traces 三大支柱，[[entities/prometheus-grafana]] 是 Metrics 工具。wiki
  将 Prometheus 归类为 Metrics 工具，但 Prometheus 生态实际上已经**超越了 Metrics 范畴**——通过 Loki（日志）和 Tempo（'
category: synthesis
tags:
- k8s
- observability
- prometheus
- metrics
- logs
- traces
- etcd
- grafana
- jaeger
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可观测性支柱 × Prometheus-Grafana 是什么
- 如何 可观测性支柱 × Prometheus-Grafana
trigger_keywords:
- 可观测性支柱
- Prometheus-Grafana
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- logging-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
relationships:
  - target: "[[entities/deployment]]"
    type: uses
  - target: "[[entities/etcd]]"
    type: uses
  - target: "[[entities/prometheus]]"
    type: uses
---

# 可观测性支柱 × [[entities/prometheus|Prometheus]]-Grafana


## 连接点

[[concepts/observability-pillars]] 描述 Metrics/Logs/Traces 三大支柱，[[entities/prometheus-grafana]] 是 Metrics 工具。wiki 将 Prometheus 归类为 Metrics 工具，但 Prometheus 生态实际上已经**超越了 Metrics 范畴**——通过 Loki（日志）和 Tempo（追踪）的标签关联，Prometheus 的标签模型正在成为可观测性三大支柱的统一查询接口。

## 共现场景

- **标签关联**：Prometheus 的指标标签（如 `pod="web-0", namespace="prod"`）与 Loki 的日志标签、Tempo 的追踪标签使用相同的键值对。Grafana 可以在同一个查询中从指标跳转到日志、再跳转到追踪
- **Exemplars**：Prometheus 的 exemplar 功能在指标中嵌入 trace ID，使得高延迟指标可以直接关联到具体的分布式追踪——Metrics 和 Traces 在数据层面连接
- **Alertmanager + 日志上下文**：Prometheus 告警触发时，Alertmanager 可以自动查询 Loki 获取该时间段的相关日志——告警与日志的自动化关联
- **Grafana Unified Dashboard**：Grafana 的 Explore 视图支持在同一个界面中查询 Metrics（Prometheus）、Logs（Loki）、Traces（Tempo）——三大支柱的可视化统一

## 交叉洞察

**核心洞察：Prometheus 的真正价值不在于 Metrics 采集，而在于它定义了可观测性的"标签关联范式"。**

传统可观测性的三大支柱是相互独立的：
- Metrics：时间序列数值（Prometheus）
- Logs：文本流（ELK、Splunk）
- Traces：请求链路（Jaeger、Zipkin）

每个支柱有自己的查询语言、存储格式和可视化工具。运维人员需要在三个系统之间手动跳转来诊断问题。

Prometheus 的标签模型统一了这三者：

```
Metrics:  http_requests_total{pod="web-0", status="500"}
Logs:     {pod="web-0"} |= "error"
Traces:   {pod="web-0", duration>1s}
```

**关键设计：标签不是 Metrics 的附属品，而是可观测性的"通用主键"。** 只要三个系统使用相同的标签集合，就可以实现无缝关联：
- 在 Grafana 中看到 HTTP 5xx 指标突增
- 点击指标跳转到同一 pod 的日志
- 在日志中发现 trace ID
- 点击 trace ID 查看完整的分布式追踪

**Prometheus 作为"可观测性路由器"：**
Prometheus 本身不存储日志和追踪，但它的标签模型定义了关联协议。Loki 和 Tempo 采用 Prometheus 的标签语法（LogQL 和 TraceQL 都基于 PromQL 风格），使得三大支柱的查询语言在语法层面统一。

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **标签爆炸** | 统一标签模型要求所有系统使用一致的标签键值对。但在实践中，应用日志、基础设施指标、服务追踪的标签命名不统一（如 `pod` vs `pod_name` vs `k8s.pod.name`）。标签标准化是大规模关联的前提，但推行标准化困难 |
| **存储成本差异** | Metrics 是高度压缩的时间序列，Logs 是大量文本，Traces 是结构化数据。三者的存储成本比例约为 1:100:10。统一查询界面可能诱导用户不加选择地关联，导致查询成本失控 |
| **查询语言的学习曲线** | PromQL、LogQL、TraceQL 虽然语法相似，但语义不同。用户在 Metrics 查询中养成的习惯（如 rate() 函数）在 Logs 查询中不成立，可能导致误解 |

## 开放问题

- **可观测性三大支柱的融合**：OpenTelemetry 试图统一 Metrics、Logs、Traces 的数据模型和采集方式。未来是否还需要"三大支柱"的概念，还是会有一个统一的"可观测性信号"？
- **标签标准的缺失**：目前缺乏跨厂商的可观测性标签标准（如 `service.name`、`deployment.name` 的统一命名）。OpenTelemetry 的语义约定正在推进，但采纳率有限


## 相关

- [[concepts/observability-pillars.md|observability-pillars]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[jaeger]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## See Also

- [[entities/etcd|etcd]] × 可观测性.md|etcd × 可观测性]]
- [[synthesis/kubeadm-cluster-operations.md|kubeadm-cluster-operations]]
- [[synthesis/声明式 API × 控制器模式.md|声明式 API × 控制器模式]]
- [[entities/deployment|Deployment]].md|控制器模式 × Deployment]]

## Related

- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.0|grafana v11.0 Release Notes]]
- grafana v8.4 Release Notes
- grafana v5.1 Release Notes
- grafana v10.1 Release Notes
- grafana v9.5 Release Notes
- grafana v6.6 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.3|grafana v12.3 Release Notes]]
- grafana v4.4 Release Notes
- grafana v8.0 Release Notes
- grafana v7.3 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.4|grafana v11.4 Release Notes]]
- grafana v6.2 Release Notes
- grafana v9.1 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.2|grafana v12.2 Release Notes]]
- grafana v4.5 Release Notes
- grafana v7.2 Release Notes
- grafana v8.1 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.5|grafana v11.5 Release Notes]]
- grafana v9.0 Release Notes
- grafana v6.3 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.4|grafana v10.4 Release Notes]]
- grafana v5.4 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.1|grafana v11.1 Release Notes]]
- grafana v8.5 Release Notes
- grafana v5.0 Release Notes
- grafana v10.0 Release Notes
- grafana v6.7 Release Notes
- grafana v9.4 Release Notes
- grafana v4.6 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.1|grafana v12.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.6|grafana v11.6 Release Notes]]
- grafana v7.1 Release Notes
- grafana v8.2 Release Notes
- grafana v9.3 Release Notes
- grafana v6.0 Release Notes
- grafana v7.5 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.2|grafana v11.2 Release Notes]]
- grafana v4.2 Release Notes
- grafana v5.3 Release Notes
- grafana v6.4 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.3|grafana v10.3 Release Notes]]
- grafana v7.4 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.3|grafana v11.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.4|grafana v12.4 Release Notes]]
- grafana v4.3 Release Notes
- grafana v5.2 Release Notes
- grafana v6.5 Release Notes
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.2|grafana v10.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.0|grafana v12.0 Release Notes]]
- grafana v8.3 Release Notes
- grafana v7.0 Release Notes
- grafana v6.1 Release Notes
- grafana v9.2 Release Notes
- [[synthesis/etcd × 可观测性|etcd × 可观测性]]
- [[log|Wiki Log]]
