---
title: Domain-8 可观测性 — 开源项目索引
description: '| **Prometheus** | 时序监控与告警 | Graduated | v3.3.0 | 56k+ | Apache-2.0
  |'
summary: '| **Prometheus** | 时序监控与告警 | Graduated | v3.3.0 | 56k+ | Apache-2.0 |'
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
tier: supporting
created: '2026-05-23'
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../平台工程/
  label: '相关知识域: 平台工程'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- 可观测性 MOC
- [[09-可观测性/README.md|Observability Domain (可观测性领域)]]
- Kubernetes 可观测性架构体系
- 指标监控体系详解
- 03 - 日志收集架构详解 (Logging Architecture)
- 分布式追踪体系
- 05 - 告警管理策略 (Alerting Management)
- 06 - 监控告警实战与最佳实践 (Monitoring Alerting Practice)
- 04 - 监控仪表板设计与最佳实践 (Monitoring Dashboards)
- 08 - 日志审计与合规管理 (Logging Auditing & Compliance)
- 05 - 事件与审计日志管理 (Events & Audit Logs)
- 07 - 监控和指标表

## See Also

- [[37-归档/domain-indexes/observability/00-open-source-projects-index-from-domain-20.md|00-open-source-projects-index-from-可观测性]]
- [[37-归档/domain-indexes/observability/01-open-source-projects-index-from-domain-21.md|00-open-source-projects-index-from-可观测性]]
- [[37-归档/domain-indexes/observability/FINAL-QUALITY-ASSESSMENT.md|FINAL-QUALITY-ASSESSMENT]]
- [[37-归档/domain-indexes/observability/MOC-from-domain-20.md|MOC-from-可观测性]]

- [[09-可观测性/README.md|返回目录]]

<!-- risk-assessed -->
