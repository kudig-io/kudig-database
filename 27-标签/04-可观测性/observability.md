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

- [[09-可观测性/01-总览/01-observability-architecture-overview|可观测性架构概览]]
- [[09-可观测性/01-总览/02-enterprise-monitoring-system|企业监控系统]]
- [[09-可观测性/01-总览/03-apm-application-performance-monitoring|APM 应用性能监控]]
- [[09-可观测性/01-总览/05-cluster-health-check|集群健康检查]]
- [[09-可观测性/01-总览/06-chaos-engineering|混沌工程]]
- [[09-可观测性/01-总览/07-security-compliance-governance|安全合规治理]]
- [[09-可观测性/01-总览/09-best-practices-case-studies|最佳实践与案例]]
- [[09-可观测性/01-总览/11-observability-tool-ecosystem|可观测性工具生态]]
- [[09-可观测性/01-总览/12-troubleshooting-overview|故障排查概览]]

## 指标 (Metrics)

- [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring|Prometheus 企业级监控]]
- [[09-可观测性/02-指标/02-monitoring-metrics-system|监控系统指标体系]]
- [[09-可观测性/02-指标/03-thanos-enterprise-metrics-federation|Thanos 企业级指标联邦]]
- [[09-可观测性/02-指标/04-monitoring-dashboards|监控仪表盘]]
- [[09-可观测性/02-指标/05-monitoring-metrics-prometheus|Prometheus 监控指标]]
- [[09-可观测性/02-指标/06-custom-metrics-adapter|自定义指标适配器]]
- [[09-可观测性/02-指标/07-enterprise-scale-monitoring|企业级大规模监控]]
- [[09-可观测性/02-指标/08-multi-cluster-monitoring-governance|多集群监控治理]]
- [[09-可观测性/02-指标/09-monitoring-cost-optimization|监控成本优化]]
- [[09-可观测性/02-指标/13-prometheus-enterprise-guide|Prometheus 企业级指南]]

## 日志 (Logging)

- [[09-可观测性/03-日志/01-elk-stack-enterprise-logging|ELK Stack 企业级日志]]
- [[09-可观测性/03-日志/02-fluentd-enterprise-log-processing|Fluentd 企业级日志处理]]
- [[09-可观测性/03-日志/03-logging-architecture|日志架构]]
- [[09-可观测性/03-日志/04-loki-enterprise-log-aggregation|Loki 企业级日志聚合]]
- [[09-可观测性/03-日志/08-logging-collection-analysis-platform|日志采集分析平台]]
- [[09-可观测性/03-日志/13-logging-audit-compliance|日志审计合规]]
- [[09-可观测性/03-日志/14-events-audit-logs|事件审计日志]]

## 链路追踪 (Tracing)

- [[09-可观测性/04-链路追踪/04-opentelemetry-distributed-tracing|OpenTelemetry 分布式追踪]]
- [[09-可观测性/04-链路追踪/05-distributed-tracing|分布式追踪]]
- [[09-可观测性/04-链路追踪/08-distributed-tracing-guide|分布式追踪指南]]

## SLO / SLI

- [[09-可观测性/06-SLO-SLI/01-slo-engineering-practice|SLO 工程实践]]
- [[09-可观测性/06-SLO-SLI/02-error-budget-policy|错误预算策略]]
- [[09-可观测性/06-SLO-SLI/03-sli-implementation-guide|SLI 实现指南]]
- [[09-可观测性/06-SLO-SLI/04-sli-definition-selection|SLI 定义选择]]
- [[09-可观测性/06-SLO-SLI/08-slo-sli-system|SLO/SLI 系统]]
- [[09-可观测性/06-SLO-SLI/09-slo-operations-guide|SLO 运营指南]]

## 告警 (Alerting)

- [[09-可观测性/05-告警/04-alerting-management|告警管理]]
- [[09-可观测性/05-告警/05-monitoring-alerting-practice|监控告警实践]]
- [[09-可观测性/05-告警/07-monitoring-playbooks|监控 Playbook]]

## 工具 (Tools)

- [[09-可观测性/07-工具/01-grafana-enterprise-observability|Grafana 企业级可观测性]]
- [[09-可观测性/07-工具/02-datadog-enterprise-apm|Datadog 企业级 APM]]
- [[09-可观测性/07-工具/04-zabbix-enterprise-monitoring|Zabbix 企业级监控]]
- [[09-可观测性/07-工具/06-troubleshooting-tools|排障工具]]
- [[09-可观测性/07-工具/07-performance-profiling-tools|性能分析工具]]

## 概念 (Concepts)

- [[22-概念/06-可观测性/observability-pillars|可观测性支柱]]
- [[22-概念/06-可观测性/k8s-observability-stack|K8s 可观测性栈]]
- [[22-概念/12-研究/observability-stack-evolution|可观测性栈演进]]
- [[22-概念/06-可观测性/multi-cluster-observability-federation|多集群可观测性联邦]]
- [[22-概念/06-可观测性/slo-monitoring-integration|SLO 监控集成]]
- [[22-概念/05-安全/security-observability-correlation|安全可观测性关联]]

## AI 可观测性 (AI Observability)

- [[15-AI基础设施/03-Agent运行时/13-agent-observability-langfuse|Agent 可观测性 Langfuse]]
- [[15-AI基础设施/02-AI-Agents/08-agent-evaluation-observability|Agent 评估可观测性]]
- [[15-AI基础设施/01-基础设施/25-llm-observability|LLM 可观测性]]
- [[22-概念/06-可观测性/ai-ml-observability|AI/ML 可观测性]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/structural-12-monitoring-observability/01-monitoring-observability-troubleshooting|监控可观测性排障]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-monitoring-alerting/DIALOGUE|监控告警对话]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-logging-pipeline/DIALOGUE|日志管道对话]]

## 研究与综合 (Research & Synthesis)

- [[25-研究/04-可靠性与运维/observability-evolution|可观测性演进]]
- [[25-研究/02-网络与安全/ebpf-observability|eBPF 可观测性]]
- [[24-综合/05-可观测性/kubernetes-prometheus|Kubernetes Prometheus]]
- [[24-综合/05-可观测性/opentelemetry-prometheus|OpenTelemetry Prometheus]]
- [[24-综合/05-可观测性/slo-observability|SLO 可观测性]]
- [[24-综合/05-可观测性/ebpf-observability|eBPF 可观测性]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/observability/prometheus|Prometheus]]
- [[17-系统基础/06-知识字典/observability/grafana|Grafana]]
- [[17-系统基础/06-知识字典/observability/loki|Loki]]
- [[17-系统基础/06-知识字典/observability/jaeger|Jaeger]]
- [[17-系统基础/06-知识字典/observability/opentelemetry|OpenTelemetry]]
- [[17-系统基础/06-知识字典/observability/thanos|Thanos]]
- [[17-系统基础/06-知识字典/observability/alertmanager|Alertmanager]]
- [[17-系统基础/06-知识字典/observability/logging|Logging]]

## 实体 (Entities)

- [[23-实体/07-可观测性/prometheus|Prometheus]]
- [[17-系统基础/06-知识字典/observability/grafana|Grafana]]
- [[23-实体/07-可观测性/jaeger|Jaeger]]
- [[23-实体/07-可观测性/opentelemetry|OpenTelemetry]]
- [[23-实体/07-可观测性/thanos|Thanos]]
- [[23-实体/07-可观测性/fluentd|Fluentd]]
- [[23-实体/15-参考与索引/cncf-observability|CNCF Observability]]
- [[23-实体/15-参考与索引/k8s-observability-ecosystem|Kubernetes Observability Ecosystem]]

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

- [[27-标签/01-核心平台/k8s|k8s]]
- [[27-标签/05-交付与运维/reliability|reliability]]
- [[27-标签/03-安全与合规/security|security]]
