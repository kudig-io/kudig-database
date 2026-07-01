---
title: 可观测性体系：指标、日志、链路追踪与混沌工程
description: '- 存储：本地 TSDB / Thanos / VictoriaMetrics / Mimir'
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
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可观测性体系：指标、日志、链路追踪与混沌工程 是什么
- 如何 可观测性体系：指标、日志、链路追踪与混沌工程
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
created: "2026-05-23"
---

# 可观测性体系

## 指标（Metrics）

**Prometheus 生态**：
- 采集：ServiceMonitor / PodMonitor（Prometheus Operator）
- 存储：本地 TSDB / Thanos / VictoriaMetrics / Mimir
- 展示：Grafana Dashboard
- 告警：Alertmanager → PagerDuty/Slack/钉钉

关键指标（RED 方法）：
- **Rate**：请求速率
- **Errors**：错误率
- **Duration**：延迟分布

## 日志（Logging）

主流方案对比：

| 方案 | 架构 | 优势 |
|------|------|------|
| EFK（Elasticsearch+Fluentd+Kibana） | 集中式 | 功能全面 |
| PLG（Promtail+Loki+Grafana） | 轻量级 | 成本低，与 Grafana 集成 |
| ClickHouse | 列式存储 | 查询极快 |

## 链路追踪（Tracing）

OpenTelemetry 统一了指标、日志、追踪三大信号：
- **SDK**：自动/手动埋点
- **Collector**：接收、处理、导出遥测数据
- **Backend**：Jaeger / Tempo / Zipkin

## 混沌工程

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

- [[concepts/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]