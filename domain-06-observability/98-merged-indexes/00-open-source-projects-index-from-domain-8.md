---
title: Domain-8 可观测性 — 开源项目索引
description: '| **Prometheus** | 时序监控与告警 | Graduated | v3.3.0 | 56k+ | Apache-2.0 |'
category: observability
tags:
- k8s
- observability
- monitoring
- logging
- tracing
- prometheus
- grafana
- jaeger
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 监控工程师
estimated_read_time: 5min
intent_queries:
- Domain-8 可观测性 — 开源项目索引 是什么
- 如何 Domain-8 可观测性 — 开源项目索引
- Kubernetes 8 observability 最佳实践
trigger_keywords:
- Domain-8
- 可观测性
- 开源项目索引
- observability
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-07-platform-engineering/
  label: '相关知识域: domain-07-platform-engineering'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---

# Domain-8 可观测性 — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Prometheus** | 时序监控与告警 | Graduated | v3.3.0 | 56k+ | Apache-2.0 |
| **Grafana** | 可视化平台 | 非 CNCF | v11.6.0 | 67k+ | AGPL-3.0 |
| **Fluentd** | 日志收集 | Graduated | v1.17.1 | 12.5k+ | Apache-2.0 |
| **Jaeger** | 分布式追踪 | Graduated | v2.5.0 | 20k+ | Apache-2.0 |
| **OpenTelemetry** | 标准化遥测 | Incubating | v1.28.0 | 25k+ | Apache-2.0 |
| **Thanos** | Prometheus 长期存储 | 非 CNCF | v0.38.0 | 13k+ | Apache-2.0 |
| **Cortex** | 多租户指标存储 | Incubating | v1.18.0 | 5.5k+ | Apache-2.0 |
| **Loki** | 日志聚合 | 非 CNCF | v3.4.0 | 25k+ | AGPL-3.0 |
| **Tempo** | 分布式追踪后端 | 非 CNCF | v2.9.0 | 4k+ | AGPL-3.0 |
| **Mimir** | 企业级指标后端 | 非 CNCF | v3.0.0 | 4k+ | AGPL-3.0 |
| **VictoriaMetrics** | 高性能时序数据库 | 非 CNCF | v1.115.0 | 13k+ | Apache-2.0 |
| **kube-state-metrics** | K8s 资源指标 | K8s SIG | v2.15.0 | 5.5k+ | Apache-2.0 |
| **metrics-server** | K8s 内置指标 | K8s SIG | v0.7.0 | - | Apache-2.0 |
| **node_exporter** | 主机指标 | Prometheus | v1.9.0 | 11k+ | Apache-2.0 |
| **cAdvisor** | 容器资源分析 | K8s SIG | v0.51.0 | 16k+ | Apache-2.0 |
| **Alertmanager** | 告警路由 | Prometheus | v0.28.0 | 6k+ | Apache-2.0 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [OpenTelemetry 文档](https://opentelemetry.io/docs/)
- [Prometheus 文档](https://prometheus.io/docs/)
- [CNCF 可观测性白皮书](https://github.com/cncf/tag-observability/blob/main/whitepaper.md)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-06-observability/MOC.md|domain-06-observability MOC]]
- [[domain-06-observability/README.md|Observability Domain (可观测性领域)]]
- [[domain-06-observability/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]]
- [[domain-06-observability/02-monitoring-metrics-system.md|指标监控体系详解]]
- [[domain-06-observability/03-logging-architecture.md|03 - 日志收集架构详解 (Logging Architecture)]]
- [[domain-06-observability/04-distributed-tracing.md|分布式追踪体系]]
- [[domain-06-observability/05-alerting-management.md|05 - 告警管理策略 (Alerting Management)]]
- [[domain-06-observability/06-monitoring-alerting-practice.md|06 - 监控告警实战与最佳实践 (Monitoring Alerting Practice)]]
- [[domain-06-observability/07-monitoring-dashboards.md|04 - 监控仪表板设计与最佳实践 (Monitoring Dashboards)]]
- [[domain-06-observability/08-logging-audit-compliance.md|08 - 日志审计与合规管理 (Logging Auditing & Compliance)]]
- [[domain-06-observability/09-events-audit-logs.md|05 - 事件与审计日志管理 (Events & Audit Logs)]]
- [[domain-06-observability/10-monitoring-metrics-prometheus.md|07 - 监控和指标表]]

## See Also

- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-20.md|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/00-open-source-projects-index-from-domain-21.md|00-open-source-projects-index-from-domain-06-observability]]
- [[domain-06-observability/98-merged-indexes/FINAL-QUALITY-ASSESSMENT.md|FINAL-QUALITY-ASSESSMENT]]
- [[domain-06-observability/98-merged-indexes/MOC-from-domain-20.md|MOC-from-domain-06-observability]]

- [[domain-06-observability/README.md|返回目录]]