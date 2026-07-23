---
title: observability
description: 可观测性标签枢纽 — 涵盖 Prometheus、Grafana、Loki、Jaeger、OpenTelemetry、SLO/SLI、告警管理、eBPF 可观测等全部可观测性领域知识
category: tag-index
tags:
- observability
- prometheus
- grafana
- opentelemetry
- logging
- tracing
- slo
tier: core
difficulty: intermediate-to-advanced
domain: observability
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# observability Tag Hub

> 可观测性领域页面 — Prometheus、Grafana、Loki、Jaeger、OpenTelemetry、SLO/SLI 等。

## 核心定义

**可观测性（Observability）** 是通过系统的外部输出（指标、日志、追踪）来理解系统内部状态的能力。在 Kubernetes 环境中，可观测性是生产运营、故障诊断、性能优化的基础。

### 三大支柱 (Three Pillars)

| 支柱 | 数据特征 | 典型工具 | 适用场景 |
|------|----------|----------|----------|
| 指标 (Metrics) | 数值型、聚合、时间序列 | Prometheus, Thanos, VictoriaMetrics | 趋势、告警、SLO |
| 日志 (Logs) | 事件型、离散、文本 | Loki, ELK, Fluent Bit | 错误详情、审计 |
| 追踪 (Traces) | 因果型、分布式、调用链 | Jaeger, Tempo, Zipkin | 延迟分析、依赖拓扑 |

### 可观测性架构全景

```
应用层:    OpenTelemetry SDK / eBPF 探针
              │            │            │
采集层:    Prometheus    Fluent Bit    OTel Collector
              │            │            │
存储层:    Thanos/Mimir    Loki        Tempo
              │            │            │
展示层:    Grafana (统一可视化 + 关联查询)
              │
决策层:    Alertmanager → On-Call → SLO 决策
```

## 生产实践要点

### 监控关键指标 (USE/RED)

| 方法 | 指标 | 适用对象 |
|------|------|----------|
| USE | Utilization, Saturation, Errors | 基础设施 (节点/磁盘/网络) |
| RED | Rate, Errors, Duration | 服务 (API/微服务) |
| Four Golden Signals | Latency, Traffic, Errors, Saturation | 全局 |

### 告警最佳实践

| 原则 | 描述 |
|------|------|
| 可操作 | 每条告警必须有 Runbook |
| 分级 | P1(立即) / P2(尽快) / P3(工作日) |
| 基于 SLO | 用错误预算消耗率触发 |
| 避免告警风暴 | 分组、抑制、静默 |
| 定期审计 | 删除无人处理的告警 |

## 总览 (Overview)

- [[可观测性/总览/01-observability-architecture-overview|可观测性架构概览]]
- [[可观测性/总览/04-enterprise-monitoring-system|企业监控系统]]
- [[可观测性/总览/06-apm-application-performance-monitoring|APM 应用性能监控]]
- [[可观测性/总览/13-cluster-health-check|集群健康检查]]
- [[可观测性/总览/14-chaos-engineering|混沌工程]]
- [[可观测性/总览/19-security-compliance-governance|安全合规治理]]
- [[可观测性/总览/22-best-practices-case-studies|最佳实践与案例]]
- [[可观测性/总览/24-observability-tool-ecosystem|可观测性工具生态]]
- [[可观测性/总览/25-troubleshooting-overview|故障排查概览]]

## 指标 (Metrics)

- [[可观测性/指标/01-prometheus-enterprise-monitoring|Prometheus 企业级监控]]
- [[可观测性/指标/02-monitoring-metrics-system|监控系统指标体系]]
- [[可观测性/指标/04-thanos-enterprise-metrics-federation|Thanos 企业级指标联邦]]
- [[可观测性/指标/07-monitoring-dashboards|监控仪表盘]]
- [[可观测性/指标/10-monitoring-metrics-prometheus|Prometheus 监控指标]]
- [[可观测性/指标/11-custom-metrics-adapter|自定义指标适配器]]
- [[可观测性/指标/15-enterprise-scale-monitoring|企业级大规模监控]]
- [[可观测性/指标/16-multi-cluster-monitoring-governance|多集群监控治理]]
- [[可观测性/指标/17-monitoring-cost-optimization|监控成本优化]]
- [[可观测性/指标/99-prometheus-enterprise-guide|Prometheus 企业级指南]]

## 日志 (Logging)

- [[可观测性/日志/01-elk-stack-enterprise-logging|ELK Stack 企业级日志]]
- [[可观测性/日志/02-fluentd-enterprise-log-processing|Fluentd 企业级日志处理]]
- [[可观测性/日志/03-logging-architecture|日志架构]]
- [[可观测性/日志/03-loki-enterprise-log-aggregation|Loki 企业级日志聚合]]
- [[可观测性/日志/05-logging-collection-analysis-platform|日志采集分析平台]]
- [[可观测性/日志/08-logging-audit-compliance|日志审计合规]]
- [[可观测性/日志/09-events-audit-logs|事件审计日志]]

## 链路追踪 (Tracing)

- [[可观测性/链路追踪/03-opentelemetry-distributed-tracing|OpenTelemetry 分布式追踪]]
- [[可观测性/链路追踪/04-distributed-tracing|分布式追踪]]
- [[可观测性/链路追踪/99-distributed-tracing-guide|分布式追踪指南]]

## SLO / SLI

- [[可观测性/SLO-SLI/01-slo-engineering-practice|SLO 工程实践]]
- [[可观测性/SLO-SLI/02-error-budget-policy|错误预算策略]]
- [[可观测性/SLO-SLI/03-sli-implementation-guide|SLI 实现指南]]
- [[可观测性/SLO-SLI/04-sli-definition-selection|SLI 定义选择]]
- [[可观测性/SLO-SLI/18-slo-sli-system|SLO/SLI 系统]]
- [[可观测性/99-slo-operations-guide|SLO 运营指南]]

## 告警 (Alerting)

- [[可观测性/告警/05-alerting-management|告警管理]]
- [[可观测性/告警/06-monitoring-alerting-practice|监控告警实践]]
- [[可观测性/告警/21-monitoring-playbooks|监控 Playbook]]

## 工具 (Tools)

- [[可观测性/工具/02-grafana-enterprise-observability|Grafana 企业级可观测性]]
- [[可观测性/工具/05-datadog-enterprise-apm|Datadog 企业级 APM]]
- [[可观测性/工具/07-zabbix-enterprise-monitoring|Zabbix 企业级监控]]
- [[可观测性/工具/26-troubleshooting-tools|排障工具]]
- [[可观测性/工具/27-performance-profiling-tools|性能分析工具]]

## 概念 (Concepts)

- [[概念/observability-pillars|可观测性支柱]]
- [[概念/k8s-observability-stack|K8s 可观测性栈]]
- [[概念/observability-stack-evolution|可观测性栈演进]]
- [[概念/multi-cluster-observability-federation|多集群可观测性联邦]]
- [[概念/slo-monitoring-integration|SLO 监控集成]]
- [[概念/security-observability-correlation|安全可观测性关联]]

## AI 可观测性 (AI Observability)

- [[AI基础设施/Agent运行时/13-agent-observability-langfuse|Agent 可观测性 Langfuse]]
- [[AI基础设施/AI-Agents/08-agent-evaluation-observability|Agent 评估可观测性]]
- [[AI基础设施/基础设施/25-llm-observability|LLM 可观测性]]
- [[概念/ai-ml-observability|AI/ML 可观测性]]

## 故障诊断 (Troubleshooting)

- [[故障诊断/高级排障/structural-12-monitoring-observability/01-monitoring-observability-troubleshooting|监控可观测性排障]]
- [[故障诊断/技能体系/skill-set/k8s-monitoring-alerting/DIALOGUE|监控告警对话]]
- [[故障诊断/技能体系/skill-set/k8s-logging-pipeline/DIALOGUE|日志管道对话]]

## 研究与综合 (Research & Synthesis)

- [[研究/observability-evolution|可观测性演进]]
- [[研究/ebpf-observability|eBPF 可观测性]]
- [[综合/kubernetes-prometheus|Kubernetes Prometheus]]
- [[综合/opentelemetry-prometheus|OpenTelemetry Prometheus]]
- [[综合/slo-observability|SLO 可观测性]]
- [[综合/ebpf-observability|eBPF 可观测性]]

## 知识字典 (Knowledge Dictionary)

- [[系统基础/知识字典/observability/prometheus|Prometheus]]
- [[系统基础/知识字典/observability/grafana|Grafana]]
- [[系统基础/知识字典/observability/loki|Loki]]
- [[系统基础/知识字典/observability/jaeger|Jaeger]]
- [[系统基础/知识字典/observability/opentelemetry|OpenTelemetry]]
- [[系统基础/知识字典/observability/thanos|Thanos]]
- [[系统基础/知识字典/observability/alertmanager|Alertmanager]]
- [[系统基础/知识字典/observability/logging|Logging]]

## 实体 (Entities)

- [[实体/prometheus|Prometheus]]
- [[实体/grafana|Grafana]]
- [[实体/jaeger|Jaeger]]
- [[实体/opentelemetry|OpenTelemetry]]
- [[实体/thanos|Thanos]]
- [[实体/fluentd|Fluentd]]
- [[实体/cncf-observability|CNCF Observability]]
- [[实体/k8s-observability-ecosystem|Kubernetes Observability Ecosystem]]

## 可观测性全景

### 三大支柱

| 支柱 | 说明 | 工具 |
|---|---|---|
| 指标 | 数值型时间序列 | Prometheus, Grafana |
| 日志 | 事件记录 | ELK, Loki |
| 追踪 | 请求链路 | Jaeger, Tempo |

### 可观测性成熟度

```
L1: 基础监控 → L2: 集中日志 → L3: 分布式追踪 → L4: SLO 驱动 → L5: AIOps
```

## 面试要点

1. **Q：可观测性 vs 监控的区别？**
   A：监控：已知问题的告警。可观测性：从外部输出推断内部状态，支持未知问题探索。

2. **Q：OpenTelemetry 的价值？**
   A：统一标准、厂商无关、自动埋点、多语言支持、社区活跃。

3. **Q：SLO 驱动的监控如何实施？**
   A：定义 SLI→设定 SLO→配置 error budget→burn rate 告警→定期回顾。

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/reliability|reliability]]
- [[标签/security|security]]
