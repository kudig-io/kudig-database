---
title: Observability
description: '**生产环境故障排查的第一入口。**'
summary: '**生产环境故障排查的第一入口。**'
category: domain
tags:
- observability
- monitoring
- logging
- tracing
- alerting
- slo
- production
- prometheus
- grafana
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Observability 是什么
- 如何 Observability
- Kubernetes 06 observability 最佳实践
trigger_keywords:
- Observability
- observability
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
- logging-basics
---



# Domain 06 — Observability（可观测性）

> **生产环境故障排查的第一入口。**

本 Domain 整合了原三个分散的可观测性相关 Domain：
- `domain-8-observability`（35 文件）
- `domain-20-enterprise-monitoring-alerting`（15 文件）
- `domain-21-logging-management-analytics`（12 文件）

## 目录结构

| 子目录 | 内容 | 文件数 |
|--------|------|--------|
| `01-overview/` | 架构概述、健康检查、混沌工程、最佳实践、排障入口 | 12 |
| `02-metrics/` | 指标系统、[[Prometheus|Prometheus]]、[[Thanos|Thanos]]、企业级监控、成本优化 | 10 |
| `03-logging/` | 日志架构、ELK、Loki、[[Fluentd|Fluentd]]、Splunk、审计合规 | 14 |
| `04-tracing/` | 分布式链路追踪、OpenTelemetry | 3 |
| `05-alerting/` | 告警管理、告警实践、On-Call Playbook | 3 |
| `06-slo-sli/` | SLO/SLI 体系 | 1 |
| `07-tools/` | 工具生态：Grafana、Datadog、Zabbix、New Relic、排障工具 | 7 |
| `98-merged-indexes/` | 原始 Domain 的元数据文件（README、MOC、索引）保留 | 12 |

## 故障排查快速入口

```
指标异常 → 02-metrics/ + 05-alerting/
日志分析 → 03-logging/
链路追踪 → 04-tracing/
健康检查 → 01-overview/13-cluster-health-check.md
排障工具 → 07-tools/26-troubleshooting-tools.md
SLO 违规 → 06-slo-sli/ + 05-alerting/21-monitoring-playbooks.md
```

## 历史元数据

原始 Domain 的索引和 MOC 保留在 `98-merged-indexes/` 中：
- `00-open-source-projects-index-from-domain-{8,20,21}.md`
- `MOC-from-domain-{8,20,21}.md`
- `README-from-domain-{8,20,21}.md`

## 与其他 Domain 的关系

- [[domain-10-troubleshooting-diagnostics/README.md|domain-10-troubleshooting-diagnostics]] — 深度排障场景
- [[domain-05-security-compliance/README.md|domain-05-security-compliance]] — 安全审计日志
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台级可观测性建设
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] — SRE 与混沌工程

## 目录内容

- [[domain-06-observability/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[domain-06-observability/06-slo-sli/18-slo-sli-system.md|18-slo-sli-system]]
- [[domain-06-observability/02-metrics/99-prometheus-enterprise-guide.md|99-prometheus-enterprise-guide]]
- [[domain-06-observability/02-metrics/10-monitoring-metrics-prometheus.md|10-monitoring-metrics-prometheus]]
- [[domain-06-observability/02-metrics/16-multi-cluster-monitoring-governance.md|16-multi-cluster-monitoring-governance]]
- [[domain-06-observability/02-metrics/02-monitoring-metrics-system.md|02-monitoring-metrics-system]]
- [[domain-06-observability/02-metrics/01-prometheus-enterprise-monitoring.md|01-prometheus-enterprise-monitoring]]
- [[domain-06-observability/02-metrics/07-monitoring-dashboards.md|07-monitoring-dashboards]]
- [[domain-06-observability/02-metrics/17-monitoring-cost-optimization.md|17-monitoring-cost-optimization]]
- [[domain-06-observability/02-metrics/15-enterprise-scale-monitoring.md|15-enterprise-scale-monitoring]]
- [[domain-06-observability/02-metrics/04-thanos-enterprise-metrics-federation.md|04-thanos-enterprise-metrics-federation]]
- [[domain-06-observability/02-metrics/11-custom-metrics-adapter.md|11-custom-metrics-adapter]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-21.md|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/UPDATED-QUALITY-REPORT.md|UPDATED-QUALITY-REPORT]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-20.md|MOC-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/README-from-domain-20.md|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/README-from-domain-8.md|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-8.md|MOC-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/README-from-domain-21.md|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-20.md|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-21.md|MOC-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/FINAL-QUALITY-ASSESSMENT.md|FINAL-QUALITY-ASSESSMENT]]
- [[domain-06-observability/98-merged-indexes/QUALITY-REPORT.md|QUALITY-REPORT]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-8.md|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/07-tools/05-datadog-enterprise-monitoring.md|05-datadog-enterprise-monitoring]]
- [[domain-06-observability/07-tools/05-datadog-enterprise-apm.md|05-datadog-enterprise-apm]]
- [[domain-06-observability/07-tools/02-grafana-enterprise-observability.md|02-grafana-enterprise-observability]]
- [[domain-06-observability/07-tools/08-new-relic-enterprise-apm.md|08-new-relic-enterprise-apm]]
- [[domain-06-observability/07-tools/26-troubleshooting-tools.md|26-troubleshooting-tools]]
- [[domain-06-observability/07-tools/07-zabbix-enterprise-monitoring.md|07-zabbix-enterprise-monitoring]]
- [[domain-06-observability/07-tools/27-performance-profiling-tools.md|27-performance-profiling-tools]]
- [[domain-06-observability/01-overview/14-chaos-engineering.md|14-chaos-engineering]]
- [[domain-06-observability/01-overview/99-kubernetes-v1.33-observability-guide.md|99-kubernetes-v1.33-observability-guide]]
- [[domain-06-observability/01-overview/25-troubleshooting-overview.md|25-troubleshooting-overview]]
- [[domain-06-observability/01-overview/19-security-compliance-governance.md|19-security-compliance-governance]]
- [[domain-06-observability/01-overview/99-java-observability-kubernetes-guide.md|99-java-observability-kubernetes-guide]]
- [[domain-06-observability/01-overview/24-observability-tool-ecosystem.md|24-observability-tool-ecosystem]]
- [[domain-06-observability/01-overview/22-best-practices-case-studies.md|22-best-practices-case-studies]]
- [[domain-06-observability/01-overview/23-enterprise-implementation-roadmap.md|23-enterprise-implementation-roadmap]]
- [[domain-06-observability/01-overview/13-cluster-health-check.md|13-cluster-health-check]]
- [[domain-06-observability/01-overview/04-enterprise-monitoring-system.md|04-enterprise-monitoring-system]]
- [[domain-06-observability/01-overview/20-high-availability-disaster-recovery.md|20-high-availability-disaster-recovery]]
- [[domain-06-observability/01-overview/01-observability-architecture-overview.md|01-observability-architecture-overview]]
- [[domain-06-observability/01-overview/06-apm-application-performance-monitoring.md|06-apm-application-performance-monitoring]]
- [[domain-06-observability/01-overview/06-elastic-stack-enterprise-observability.md|06-elastic-stack-enterprise-observability]]
- [[domain-06-observability/05-alerting/06-monitoring-alerting-practice.md|06-monitoring-alerting-practice]]
- [[domain-06-observability/05-alerting/21-monitoring-playbooks.md|21-monitoring-playbooks]]
- [[domain-06-observability/05-alerting/05-alerting-management.md|05-alerting-management]]
- [[domain-06-observability/03-logging/02-fluentd-enterprise-log-processing.md|02-fluentd-enterprise-log-processing]]
- [[domain-06-observability/03-logging/03-logging-architecture.md|03-logging-architecture]]
- [[domain-06-observability/03-logging/04-enterprise-log-governance-compliance.md|04-enterprise-log-governance-compliance]]
- [[domain-06-observability/03-logging/04-graylog-enterprise-logging.md|04-graylog-enterprise-logging]]
- [[domain-06-observability/03-logging/05-splunk-enterprise-log-analytics.md|05-splunk-enterprise-log-analytics]]
- [[domain-06-observability/03-logging/08-logging-audit-compliance.md|08-logging-audit-compliance]]
- [[domain-06-observability/03-logging/12-logging-auditing.md|12-logging-auditing]]
- [[domain-06-observability/03-logging/04-splunk-enterprise-siem.md|04-splunk-enterprise-siem]]
- [[domain-06-observability/03-logging/06-elastic-stack-enterprise-logging.md|06-elastic-stack-enterprise-logging]]
- [[domain-06-observability/03-logging/03-loki-enterprise-log-aggregation.md|03-loki-enterprise-log-aggregation]]
- [[domain-06-observability/03-logging/05-logging-collection-analysis-platform.md|05-logging-collection-analysis-platform]]
- [[domain-06-observability/03-logging/09-events-audit-logs.md|09-events-audit-logs]]
- [[domain-06-observability/03-logging/06-loggly-cloud-log-management.md|06-loggly-cloud-log-management]]
- [[domain-06-observability/03-logging/05-real-time-analytics-business-insights.md|05-real-time-analytics-business-insights]]
- [[domain-06-observability/03-logging/01-elk-stack-enterprise-logging.md|01-elk-stack-enterprise-logging]]
- [[domain-06-observability/04-tracing/04-distributed-tracing.md|04-distributed-tracing]]
- [[domain-06-observability/04-tracing/99-distributed-tracing-guide.md|99-distributed-tracing-guide]]
- [[domain-06-observability/04-tracing/03-opentelemetry-distributed-tracing.md|03-opentelemetry-distributed-tracing]]

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]

## 相关合成分析

- [[concepts/chaos-engineering-observability.md|chaos-engineering-observability]]
