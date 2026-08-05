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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-06-observability/03-slo-sli/01-slo-sli-system|18-slo-sli-system]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/09-prometheus-enterprise-guide|99-prometheus-enterprise-guide]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/04-monitoring-metrics-prometheus|10-monitoring-metrics-prometheus]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/07-multi-cluster-monitoring-governance|16-multi-cluster-monitoring-governance]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/01-monitoring-metrics-system|02-monitoring-metrics-system]]
- [[domain-06-observability/指标/01-prometheus-enterprise-monitoring.md|01-prometheus-enterprise-monitoring]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/03-monitoring-dashboards|07-monitoring-dashboards]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/08-monitoring-cost-optimization|17-monitoring-cost-optimization]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/06-enterprise-scale-monitoring|15-enterprise-scale-monitoring]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/02-thanos-enterprise-metrics-federation|04-thanos-enterprise-metrics-federation]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/02-metrics/05-custom-metrics-adapter|11-custom-metrics-adapter]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/98-merged-indexes/01-open-source-projects-index-from-domain-21|00-open-source-projects-index-from-domain-06-observability]]
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
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/98-merged-indexes/02-open-source-projects-index-from-domain-8|00-open-source-projects-index-from-domain-06-observability]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/03-datadog-enterprise-monitoring|05-datadog-enterprise-monitoring]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/02-datadog-enterprise-apm|05-datadog-enterprise-apm]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/01-grafana-enterprise-observability|02-grafana-enterprise-observability]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/05-new-relic-enterprise-apm|08-new-relic-enterprise-apm]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/06-troubleshooting-tools|26-troubleshooting-tools]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/04-zabbix-enterprise-monitoring|07-zabbix-enterprise-monitoring]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/07-tools/07-performance-profiling-tools|27-performance-profiling-tools]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/05-chaos-engineering|14-chaos-engineering]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/13-kubernetes-v1.33-observability-guide|99-kubernetes-v1.33-observability-guide]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/11-troubleshooting-overview|25-troubleshooting-overview]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/06-security-compliance-governance|19-security-compliance-governance]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/12-java-observability-kubernetes-guide|99-java-observability-kubernetes-guide]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/10-observability-tool-ecosystem|24-observability-tool-ecosystem]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/08-best-practices-case-studies|22-best-practices-case-studies]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/09-enterprise-implementation-roadmap|23-enterprise-implementation-roadmap]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/04-cluster-health-check|13-cluster-health-check]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/01-enterprise-monitoring-system|04-enterprise-monitoring-system]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/07-high-availability-disaster-recovery|20-high-availability-disaster-recovery]]
- [[domain-06-observability/总览/01-observability-architecture-overview.md|01-observability-architecture-overview]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/02-apm-application-performance-monitoring|06-apm-application-performance-monitoring]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/01-overview/03-elastic-stack-enterprise-observability|06-elastic-stack-enterprise-observability]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/05-alerting/05-monitoring-alerting-practice|06-monitoring-alerting-practice]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/05-alerting/06-monitoring-playbooks|21-monitoring-playbooks]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/05-alerting/04-alerting-management|05-alerting-management]]
- [[domain-06-observability/日志/02-fluentd-enterprise-log-processing.md|02-fluentd-enterprise-log-processing]]
- [[domain-06-observability/日志/03-logging-architecture.md|03-logging-architecture]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/05-enterprise-log-governance-compliance|04-enterprise-log-governance-compliance]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/06-graylog-enterprise-logging|04-graylog-enterprise-logging]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/10-splunk-enterprise-log-analytics|05-splunk-enterprise-log-analytics]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/13-logging-audit-compliance|08-logging-audit-compliance]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/15-logging-auditing|12-logging-auditing]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/07-splunk-enterprise-siem|04-splunk-enterprise-siem]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/11-elastic-stack-enterprise-logging|06-elastic-stack-enterprise-logging]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/04-loki-enterprise-log-aggregation|03-loki-enterprise-log-aggregation]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/08-logging-collection-analysis-platform|05-logging-collection-analysis-platform]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/14-events-audit-logs|09-events-audit-logs]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/12-loggly-cloud-log-management|06-loggly-cloud-log-management]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/03-logging/09-real-time-analytics-business-insights|05-real-time-analytics-business-insights]]
- [[domain-06-observability/日志/01-elk-stack-enterprise-logging.md|01-elk-stack-enterprise-logging]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/04-tracing/05-distributed-tracing|04-distributed-tracing]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/04-tracing/06-distributed-tracing-guide|99-distributed-tracing-guide]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-06-observability/04-tracing/04-opentelemetry-distributed-tracing|03-opentelemetry-distributed-tracing]]

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]

## 相关合成分析

- [[concepts/chaos-engineering-observability.md|chaos-engineering-observability]]


<!-- risk-assessed -->
