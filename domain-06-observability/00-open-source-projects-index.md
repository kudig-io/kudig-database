---
title: Observability Open Source Projects Index
description: '## 指标与监控'
category: reference
tags:
- observability
- monitoring
- logging
- tracing
- open-source
- index
- prometheus
- grafana
- jaeger
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Observability Open Source Projects Index 是什么
- 如何 Observability Open Source Projects Index
- Kubernetes 06 observability 最佳实践
trigger_keywords:
- Observability
- Open
- Source
- Projects
- Index
- observability
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
---

# 可观测性开源项目索引

> 本索引合并了原 `domain-8-observability`、`domain-20-enterprise-monitoring-alerting`、`domain-21-logging-management-analytics` 三个域的开源项目信息。

## 指标与监控

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| Prometheus | 时序数据库 | 云原生指标采集与存储 | `02-metrics/01-prometheus-enterprise-monitoring.md` |
| Thanos | 指标联邦 | Prometheus 长期存储与全局查询 | `02-metrics/04-thanos-enterprise-metrics-federation.md` |
| Grafana | 可视化 | 监控仪表盘与告警 | `07-tools/02-grafana-enterprise-observability.md` |
| Datadog | SaaS 监控 | 企业级 APM 与基础设施监控 | `07-tools/05-datadog-enterprise-monitoring.md` |
| Zabbix | 企业监控 | 传统基础设施监控 | `07-tools/07-zabbix-enterprise-monitoring.md` |
| New Relic | SaaS APM | 应用性能监控 | `07-tools/08-new-relic-enterprise-apm.md` |

## 日志管理

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| ELK Stack | 日志平台 | Elasticsearch + Logstash + Kibana | `03-logging/01-elk-stack-enterprise-logging.md` |
| Loki | 日志聚合 | Grafana 生态的轻量级日志系统 | `03-logging/03-loki-enterprise-log-aggregation.md` |
| Fluentd | 日志收集 | 统一日志层 | `03-logging/02-fluentd-enterprise-log-processing.md` |
| Graylog | 日志管理 | 企业级日志分析与告警 | `03-logging/04-graylog-enterprise-logging.md` |
| Splunk | 数据分析 | 企业级 SIEM 与日志分析 | `03-logging/04-splunk-enterprise-siem.md` |

## 分布式追踪

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| OpenTelemetry | 标准/框架 | 可观测性数据采集标准 | `04-tracing/03-opentelemetry-distributed-tracing.md` |
| Jaeger | 追踪系统 | 分布式链路追踪 | （见 tracing 相关文档） |
| Zipkin | 追踪系统 | Twitter 开源的分布式追踪 | （见 tracing 相关文档） |

## 原始索引保留

更详细的索引见：
- `98-merged-indexes/00-open-source-projects-index-from-domain-8.md`
- `98-merged-indexes/00-open-source-projects-index-from-domain-20.md`
- `98-merged-indexes/00-open-source-projects-index-from-domain-21.md`

## See Also

- [[domain-06-observability/98-merged-indexes/README-from-domain-8.md|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/UPDATED-QUALITY-REPORT.md|UPDATED-QUALITY-REPORT]]
- [[domain-06-observability/01-overview/01-observability-architecture-overview.md|01-observability-architecture-overview]]
- [[domain-06-observability/01-overview/06-elastic-stack-enterprise-observability.md|06-elastic-stack-enterprise-observability]]

- [[domain-06-observability/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
