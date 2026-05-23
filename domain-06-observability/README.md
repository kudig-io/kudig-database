---
title: Observability
description: '**生产环境故障排查的第一入口。**'
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
created: "2026-05-23"
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

- [[domain-10-troubleshooting-diagnostics/README|domain-10-troubleshooting-diagnostics]] — 深度排障场景
- [[domain-05-security-compliance/README|domain-05-security-compliance]] — 安全审计日志
- [[domain-07-platform-engineering/README|domain-07-platform-engineering]] — 平台级可观测性建设
- [[domain-09-reliability-engineering/README|domain-09-reliability-engineering]] — SRE 与混沌工程

## 目录内容

- [[domain-06-observability/00-open-source-projects-index|00-open-source-projects-index]]
- [[domain-06-observability/06-slo-sli/18-slo-sli-system|18-slo-sli-system]]
- [[domain-06-observability/02-metrics/99-prometheus-enterprise-guide|99-prometheus-enterprise-guide]]
- [[domain-06-observability/02-metrics/10-monitoring-metrics-prometheus|10-monitoring-metrics-prometheus]]
- [[domain-06-observability/02-metrics/16-multi-cluster-monitoring-governance|16-multi-cluster-monitoring-governance]]
- [[domain-06-observability/02-metrics/02-monitoring-metrics-system|02-monitoring-metrics-system]]
- [[domain-06-observability/02-metrics/01-prometheus-enterprise-monitoring|01-prometheus-enterprise-monitoring]]
- [[domain-06-observability/02-metrics/07-monitoring-dashboards|07-monitoring-dashboards]]
- [[domain-06-observability/02-metrics/17-monitoring-cost-optimization|17-monitoring-cost-optimization]]
- [[domain-06-observability/02-metrics/15-enterprise-scale-monitoring|15-enterprise-scale-monitoring]]
- [[domain-06-observability/02-metrics/04-thanos-enterprise-metrics-federation|04-thanos-enterprise-metrics-federation]]
- [[domain-06-observability/02-metrics/11-custom-metrics-adapter|11-custom-metrics-adapter]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-21|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/UPDATED-QUALITY-REPORT|UPDATED-QUALITY-REPORT]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-20|MOC-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/README-from-domain-20|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/README-from-domain-8|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-8|MOC-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/README-from-domain-21|README-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-20|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-21|MOC-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/FINAL-QUALITY-ASSESSMENT|FINAL-QUALITY-ASSESSMENT]]
- [[domain-06-observability/98-merged-indexes/QUALITY-REPORT|QUALITY-REPORT]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-8|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/07-tools/05-datadog-enterprise-monitoring|05-datadog-enterprise-monitoring]]
- [[domain-06-observability/07-tools/05-datadog-enterprise-apm|05-datadog-enterprise-apm]]
- [[domain-06-observability/07-tools/02-grafana-enterprise-observability|02-grafana-enterprise-observability]]
- [[domain-06-observability/07-tools/08-new-relic-enterprise-apm|08-new-relic-enterprise-apm]]
- [[domain-06-observability/07-tools/26-troubleshooting-tools|26-troubleshooting-tools]]
- [[domain-06-observability/07-tools/07-zabbix-enterprise-monitoring|07-zabbix-enterprise-monitoring]]
- [[domain-06-observability/07-tools/27-performance-profiling-tools|27-performance-profiling-tools]]
- [[domain-06-observability/01-overview/14-chaos-engineering|14-chaos-engineering]]
- [[domain-06-observability/01-overview/99-kubernetes-v1.33-observability-guide|99-kubernetes-v1.33-observability-guide]]
- [[domain-06-observability/01-overview/25-troubleshooting-overview|25-troubleshooting-overview]]
- [[domain-06-observability/01-overview/19-security-compliance-governance|19-security-compliance-governance]]
- [[domain-06-observability/01-overview/99-java-observability-kubernetes-guide|99-java-observability-kubernetes-guide]]
- [[domain-06-observability/01-overview/24-observability-tool-ecosystem|24-observability-tool-ecosystem]]
- [[domain-06-observability/01-overview/22-best-practices-case-studies|22-best-practices-case-studies]]
- [[domain-06-observability/01-overview/23-enterprise-implementation-roadmap|23-enterprise-implementation-roadmap]]
- [[domain-06-observability/01-overview/13-cluster-health-check|13-cluster-health-check]]
- [[domain-06-observability/01-overview/04-enterprise-monitoring-system|04-enterprise-monitoring-system]]
- [[domain-06-observability/01-overview/20-high-availability-disaster-recovery|20-high-availability-disaster-recovery]]
- [[domain-06-observability/01-overview/01-observability-architecture-overview|01-observability-architecture-overview]]
- [[domain-06-observability/01-overview/06-apm-application-performance-monitoring|06-apm-application-performance-monitoring]]
- [[domain-06-observability/01-overview/06-elastic-stack-enterprise-observability|06-elastic-stack-enterprise-observability]]
- [[domain-06-observability/05-alerting/06-monitoring-alerting-practice|06-monitoring-alerting-practice]]
- [[domain-06-observability/05-alerting/21-monitoring-playbooks|21-monitoring-playbooks]]
- [[domain-06-observability/05-alerting/05-alerting-management|05-alerting-management]]
- [[domain-06-observability/03-logging/02-fluentd-enterprise-log-processing|02-fluentd-enterprise-log-processing]]
- [[domain-06-observability/03-logging/03-logging-architecture|03-logging-architecture]]
- [[domain-06-observability/03-logging/04-enterprise-log-governance-compliance|04-enterprise-log-governance-compliance]]
- [[domain-06-observability/03-logging/04-graylog-enterprise-logging|04-graylog-enterprise-logging]]
- [[domain-06-observability/03-logging/05-splunk-enterprise-log-analytics|05-splunk-enterprise-log-analytics]]
- [[domain-06-observability/03-logging/08-logging-audit-compliance|08-logging-audit-compliance]]
- [[domain-06-observability/03-logging/12-logging-auditing|12-logging-auditing]]
- [[domain-06-observability/03-logging/04-splunk-enterprise-siem|04-splunk-enterprise-siem]]
- [[domain-06-observability/03-logging/06-elastic-stack-enterprise-logging|06-elastic-stack-enterprise-logging]]
- [[domain-06-observability/03-logging/03-loki-enterprise-log-aggregation|03-loki-enterprise-log-aggregation]]
- [[domain-06-observability/03-logging/05-logging-collection-analysis-platform|05-logging-collection-analysis-platform]]
- [[domain-06-observability/03-logging/09-events-audit-logs|09-events-audit-logs]]
- [[domain-06-observability/03-logging/06-loggly-cloud-log-management|06-loggly-cloud-log-management]]
- [[domain-06-observability/03-logging/05-real-time-analytics-business-insights|05-real-time-analytics-business-insights]]
- [[domain-06-observability/03-logging/01-elk-stack-enterprise-logging|01-elk-stack-enterprise-logging]]
- [[domain-06-observability/04-tracing/04-distributed-tracing|04-distributed-tracing]]
- [[domain-06-observability/04-tracing/99-distributed-tracing-guide|99-distributed-tracing-guide]]
- [[domain-06-observability/04-tracing/03-opentelemetry-distributed-tracing|03-opentelemetry-distributed-tracing]]

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]

## 相关合成分析

- [[synthesis/chaos-engineering-observability|chaos-engineering-observability]]
