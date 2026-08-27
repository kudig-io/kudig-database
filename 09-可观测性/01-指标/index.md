---
title: Metrics & Prometheus
description: 指标知识域 — Prometheus 企业级监控、Thanos 联邦、自定义指标、Dashboard 设计、多集群监控治理
category: subdomain
tags:
- prometheus
- metrics
- thanos
- monitoring
- dashboard
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 指标与 Prometheus Metrics

> 以 Prometheus 为核心的指标采集、存储、查询、可视化与告警全链路。

## 指标类型

| 类型 | 说明 | 典型用途 |
|------|------|----------|
| Counter | 只增不减的累计值 | 请求总数、错误计数 |
| Gauge | 可增可减的瞬时值 | 内存使用、当前连接数 |
| Histogram | 分布统计（bucket） | 请求延迟分布 |
| Summary | 分位数统计 | P99 延迟 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring.md\|Prometheus 企业监控]] | 架构/存储/高可用 | advanced |
| [[09-可观测性/02-指标/02-monitoring-metrics-system.md\|指标体系设计]] | 指标命名/分类/治理 | intermediate |
| [[09-可观测性/01-指标/03-thanos-enterprise-metrics-federation.md\|Thanos 联邦]] | 多集群指标聚合与长期存储 | advanced |
| [[09-可观测性/01-指标/04-monitoring-dashboards.md\|Dashboard 设计]] | Grafana 仪表盘设计原则 | intermediate |
| [[09-可观测性/01-指标/05-monitoring-metrics-prometheus.md\|Prometheus 实践]] | PromQL/服务发现/告警规则 | intermediate |
| [[09-可观测性/01-指标/06-custom-metrics-adapter.md\|自定义指标适配器]] | HPA 自定义指标集成 | advanced |
| [[09-可观测性/01-指标/07-enterprise-scale-monitoring.md\|企业级监控]] | 大规模监控架构 | advanced |
| [[09-可观测性/01-指标/08-multi-cluster-monitoring-governance.md\|多集群监控治理]] | 联邦/远程写入/统一视图 | advanced |
| [[09-可观测性/01-指标/09-monitoring-cost-optimization.md\|监控成本优化]] | 指标瘦身/降采样/存储策略 | intermediate |
| [[09-可观测性/01-指标/10-cost-optimization-observability.md\|可观测性成本优化]] | 成本治理与策略 | intermediate |
| [[09-可观测性/01-指标/11-kube-state-metrics-deep-dive.md\|kube-state-metrics]] | 对象状态指标 pipeline | advanced |
| [[09-可观测性/01-指标/12-cadvisor-kubelet-metrics.md\|cAdvisor 与 kubelet 指标]] | 容器级资源指标采集 | advanced |
| [[09-可观测性/01-指标/13-prometheus-enterprise-guide.md\|Prometheus 指南]] | 生产环境完整指南 | advanced |

## Prometheus 生产检查清单

- [ ] 启用远程写入（Thanos/Cortex/Mimir）保障长期存储
- [ ] 配置 recording rules 预计算高频查询
- [ ] 指标命名遵循 `namespace_subsystem_name_unit` 规范
- [ ] 设置合理的 scrape_interval（15s-60s）
- [ ] 使用 ServiceMonitor CRD 管理服务发现
- [ ] 定期清理无用指标（降低存储成本）

## Related

- [[09-可观测性/05-告警/index.md|告警 Alerting]]
- [[09-可观测性/06-SLO-SLI/index.md|SLO & SLI]]
- [[09-可观测性/07-工具/index.md|可观测性工具]]
