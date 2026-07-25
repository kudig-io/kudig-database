---
title: 可观测性体系：指标、日志、链路追踪与混沌工程
description: '- 存储：本地 TSDB / Thanos / VictoriaMetrics / Mimir'
summary: '- 存储：本地 TSDB / Thanos / VictoriaMetrics / Mimir'
category: reference
tags:
- k8s
- observability
- metrics
- logging
- tracing
- chaos-engineering
- prometheus
- opentelemetry
- grafana
- jaeger
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可观测性体系：指标、日志、链路追踪与混沌工程 是什么
- 如何 可观测性体系：指标、链路追踪与混沌工程
trigger_keywords:
- 可观测性体系：指标
- 日志
- 链路追踪与混沌工程
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



# 可观测性体系

> **类别**: Core Concept | **三大支柱**: Metrics, Logging, Tracing

## 概述

可观测性体系是 Kubernetes 生产化运维的核心支撑，涵盖指标（Metrics）、日志（Logging）、链路追踪（Tracing）和混沌工程（Chaos Engineering）四大领域。可观测性的目标是让运维和开发团队随时了解系统的内部状态——不仅要知道"出了什么问题"（监控），还要理解"为什么出问题"（诊断）。Prometheus + Grafana 构成了云原生指标监控的事实标准，OpenTelemetry 统一了三大遥测信号的采集标准，Jaeger/Tempo 提供分布式追踪能力，Chaos Mesh/LitmusChaos 则通过混沌工程验证系统韧性。成熟的可观测性体系遵循 RED 方法（Rate、Errors、Duration）和 USE 方法（Utilization、Saturation、Errors）。

## 核心能力

- **指标采集（Metrics）**: Prometheus ServiceMonitor/PodMonitor + remote write 长期存储
- **日志管理（Logging）**: Fluentd/Fluent Bit + Loki/Elasticsearch + Kibana/Grafana
- **链路追踪（Tracing）**: OpenTelemetry SDK + Collector + Jaeger/Tempo 后端
- **告警（Alerting）**: Alertmanager + PagerDuty/Slack/钉钉 多通道告警
- **混沌工程（Chaos）**: Chaos Mesh/LitmusChaos 主动验证系统韧性
- **统一遥测**: OpenTelemetry 统一 Metrics + Logging + Tracing 三大信号

## 架构

可观测性体系由数据采集、处理、存储和展示四层组成：

**指标链路**：
- 采集：Prometheus（pull）/ OTel Collector（push）→ 存储：本地 TSDB / Thanos / VictoriaMetrics / Mimir → 展示：Grafana
**日志链路**：
- 采集：Fluent Bit（DaemonSet）→ 缓冲：Kafka → 存储：Elasticsearch/Loki → 展示：Kibana/Grafana
**追踪链路**：
- 埋点：OTel SDK → 收集：OTel Collector → 存储：Jaeger/Tempo → 展示：Jaeger UI/Grafana

关键指标（RED 方法）：Rate（请求速率）、Errors（错误率）、Duration（延迟分布）
关键指标（USE 方法）：Utilization（利用率）、Saturation（饱和度）、Errors（错误数）

## K8s 集成

可观测性组件以 Kubernetes 原生方式部署。Prometheus Operator 通过 ServiceMonitor/PodMonitor CRD 自动发现和配置监控目标。Fluent Bit 以 DaemonSet 运行在每个节点，采集容器 stdout/stderr 日志。OpenTelemetry Collector 以 Deployment 或 DaemonSet 运行，接收 OTLP 数据并导出到后端。Jaeger/Tempo Operator 管理 tracing 后端。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Labels、Annotations 和 API 资源发现机制深度集成。

## 生产场景

1. **SLO 监控告警**: 基于 RED/USE 方法定义 SLO 指标，配置多级告警
2. **故障根因定位**: 通过 Trace ID 关联 Metrics → Logs → Traces，快速定位问题
3. **性能瓶颈分析**: 通过 Trace 分析微服务调用链，发现延迟瓶颈
4. **韧性验证**: 定期运行混沌实验，验证系统的容错和恢复能力

## 安装

```bash
# 安装 Prometheus + Grafana (kube-prometheus-stack)
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install monitoring prometheus-community/kube-prometheus-stack -n monitoring --create-namespace

# 安装 Loki + Fluent Bit (日志)
helm repo add grafana https://grafana.github.io/helm-charts
helm install loki grafana/loki-stack -n monitoring --set fluent-bit.enabled=true

# 安装 OpenTelemetry Collector
helm repo add open-telemetry https://open-telemetry.github.io/opentelemetry-helm-charts
helm install otel-collector open-telemetry/opentelemetry-collector -n monitoring

# 安装 Jaeger
helm repo add jaegertracing https://jaegertracing.github.io/helm-charts
helm install jaeger jaegertracing/jaeger -n monitoring
```

## 对比

| 方案 | 架构 | 优势 | 适用场景 |
|------|------|------|----------|
| EFK | 集中式索引 | 功能全面 | 日志搜索 |
| PLG (Promtail+Loki+Grafana) | 轻量标签 | 成本低 | 大规模日志 |
| ClickHouse | 列式存储 | 查询极快 | 高性能分析 |

## 详细组件

### 指标（Metrics）

**Prometheus 生态**：
- 采集：ServiceMonitor / PodMonitor（Prometheus Operator）
- 存储：本地 TSDB / Thanos / VictoriaMetrics / Mimir
- 展示：Grafana Dashboard
- 告警：Alertmanager → PagerDuty/Slack/钉钉

### 日志（Logging）

主流方案对比：

| 方案 | 架构 | 优势 |
|------|------|------|
| EFK（Elasticsearch+Fluentd+Kibana） | 集中式 | 功能全面 |
| PLG（Promtail+Loki+Grafana） | 轻量级 | 成本低，与 Grafana 集成 |
| ClickHouse | 列式存储 | 查询极快 |

### 链路追踪（Tracing）

OpenTelemetry 统一了指标、日志、追踪三大信号：
- **SDK**：自动/手动埋点
- **Collector**：接收、处理、导出遥测数据
- **Backend**：Jaeger / Tempo / Zipkin

### 混沌工程

在生产前验证系统韧性：
- **Chaos Mesh**：Pod 故障注入、网络延迟/丢包、IO 问题
- **LitmusChaos**：场景库丰富，支持 GitOps 集成

---

> 来源：.zread/wiki/drafts/12-ke-guan-ce-xing-jian-kong-zhi-biao-ri-zhi-shen-ji-lian-lu-zhui-zong-yu-hun-dun-gong-cheng.md

## Related

- [[fluentd]] — Fluentd
- [[thanos]] — Thanos
- [[jaeger]] — Jaeger
- [[litmus]] — LitmusChaos
- [[prometheus]] — Prometheus

- [[22-概念/11-交叉分析/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]

<!-- risk-assessed -->
