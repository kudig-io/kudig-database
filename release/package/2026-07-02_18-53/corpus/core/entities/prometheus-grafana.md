---
title: Prometheus and Grafana
description: Prometheus and Grafana — Kubernetes 生产运维知识库
summary: Prometheus and Grafana — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- monitoring
- prometheus
- grafana
- metrics
- alerting
- etcd
- jaeger
- cilium
- elasticsearch
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Prometheus and Grafana 是什么
- 如何 Prometheus and Grafana
trigger_keywords:
- Prometheus
- and
- Grafana
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Prometheus and Grafana

## Prometheus

Prometheus is the de facto standard monitoring system for Kubernetes, providing:

- **Pull-based architecture**: Scrapes metrics from targets via HTTP
- **Time-series database**: Local storage with configurable retention
- **PromQL**: Powerful query language for aggregation, filtering, and mathematical operations
- **[[Service|Service]] Discovery**: Native K8s service discovery for automatic target detection
- **Alertmanager**: Handles alert deduplication, grouping, inhibition, and routing

### Key Metrics Types

| Type | Description | Example |
|------|-------------|---------|
| Counter | Monotonically increasing value | http_requests_total |
| Gauge | Current value that goes up/down | node_memory_available_bytes |
| Histogram | Value distribution in buckets | http_request_duration_seconds_bucket |
| Summary | Pre-computed quantiles | http_request_duration_seconds |

### HA Architecture

Prometheus servers are deployed in pairs (active-active) with identical scrape configurations. Thanos or Cortex provides global query view and long-term storage via object store (S3/GCS).

## Grafana

Grafana provides visualization and dashboard capabilities:
- **Multi-source**: Connects to Prometheus, Loki, Tempo, Elasticsearch, and many more
- **Dashboard templating**: Variable-driven dashboards for multi-cluster/multi-namespace views
- **Alerting**: Built-in alert rules with notification channels
- **Enterprise features**: RBAC, SSO, audit logging, reporting

## Standard K8s Monitoring Stack

```
Prometheus (metrics collection + storage)
    -> Alertmanager (alert routing + notification)
    -> Grafana (visualization + dashboards)

Loki (log aggregation)
    -> Promtail/Fluentd (log collection)
    -> Grafana (log visualization)

Tempo/Jaeger (distributed tracing)
    -> OpenTelemetry Collector (trace ingestion)
    -> Grafana/Jaeger UI (trace visualization)
```

## Related
- [[concepts/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合
- [[concepts/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合
- [[concepts/控制器模式 × 可观测性.md|控制器模式 × 可观测性]] — 综合
- [[concepts/CRD × 可观测性.md|CRD × 可观测性]] — 综合
- [[concepts/etcd × 可观测性.md|etcd × 可观测性]] — 综合
- [[concepts/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]] — 综合

- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/production-operations-best-practices.md|production-operations-best-practices]] — Production Operations Best Practices
- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[concepts/production-operations-best-practices.md|Production Operations Best Practices]]
- [[concepts/microservice-resilience-patterns.md|Microservice Resilience Patterns]]

- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- 02-grafana-enterprise-observability
- RELEASE-NOTES-0.12
- RELEASE-NOTES-2.32
- RELEASE-NOTES-2.22
- RELEASE-NOTES-2.47
- RELEASE-NOTES-2.16
- RELEASE-NOTES-2.36
- RELEASE-NOTES-2.53
- RELEASE-NOTES-0.16
- RELEASE-NOTES-2.12
- RELEASE-NOTES-2.43
- RELEASE-NOTES-2.26
- RELEASE-NOTES-2.37
- RELEASE-NOTES-2.52
- RELEASE-NOTES-0.17
- RELEASE-NOTES-2.13
- RELEASE-NOTES-2.42
- RELEASE-NOTES-2.27
- RELEASE-NOTES-0.13
- RELEASE-NOTES-1.8
- RELEASE-NOTES-2.33
- RELEASE-NOTES-2.23
- RELEASE-NOTES-2.46
- RELEASE-NOTES-2.17
- RELEASE-NOTES-2.4
- RELEASE-NOTES-0.18
- RELEASE-NOTES-1.3
- RELEASE-NOTES-2.38
- RELEASE-NOTES-2.28
- RELEASE-NOTES-3.5
- RELEASE-NOTES-1.7
- RELEASE-NOTES-2.0
- RELEASE-NOTES-2.18
- RELEASE-NOTES-2.49
- RELEASE-NOTES-3.1
- RELEASE-NOTES-1.6
- RELEASE-NOTES-2.1
- RELEASE-NOTES-2.19
- RELEASE-NOTES-2.48
- RELEASE-NOTES-3.0
- RELEASE-NOTES-2.5
- RELEASE-NOTES-0.19
- RELEASE-NOTES-1.2
- RELEASE-NOTES-2.39
- RELEASE-NOTES-2.29
- RELEASE-NOTES-3.4
- RELEASE-NOTES-1.5
- RELEASE-NOTES-2.2
- RELEASE-NOTES-3.3
- RELEASE-NOTES-2.6
- RELEASE-NOTES-1.1
- RELEASE-NOTES-3.7
- RELEASE-NOTES-3.10
- RELEASE-NOTES-2.7
- RELEASE-NOTES-1.0
- RELEASE-NOTES-3.6
- RELEASE-NOTES-3.11
- RELEASE-NOTES-1.4
- RELEASE-NOTES-2.3
- RELEASE-NOTES-3.2
- RELEASE-NOTES-0.20
- RELEASE-NOTES-2.34
- RELEASE-NOTES-2.8
- RELEASE-NOTES-2.51
- RELEASE-NOTES-0.14
- RELEASE-NOTES-2.41
- RELEASE-NOTES-2.10
- RELEASE-NOTES-3.9
- RELEASE-NOTES-2.24
- RELEASE-NOTES-2.55
- RELEASE-NOTES-2.30
- RELEASE-NOTES-2.20
- RELEASE-NOTES-2.14
- RELEASE-NOTES-2.45
- RELEASE-NOTES-2.54
- RELEASE-NOTES-0.11
- RELEASE-NOTES-2.31
- RELEASE-NOTES-2.21
- RELEASE-NOTES-2.15
- RELEASE-NOTES-2.44
- RELEASE-NOTES-2.35
- RELEASE-NOTES-2.9
- RELEASE-NOTES-2.50
- RELEASE-NOTES-0.15
- RELEASE-NOTES-2.40
- RELEASE-NOTES-2.11
- RELEASE-NOTES-3.8
- RELEASE-NOTES-2.25
- RELEASE-NOTES-1.9
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.0.md|RELEASE-NOTES-11.0]]
- RELEASE-NOTES-8.4
- RELEASE-NOTES-4.0
- RELEASE-NOTES-5.1
- RELEASE-NOTES-10.1
- RELEASE-NOTES-9.5
- RELEASE-NOTES-6.6
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-12.3.md|RELEASE-NOTES-12.3]]
- RELEASE-NOTES-4.4
- RELEASE-NOTES-8.0
- RELEASE-NOTES-7.3
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.4.md|RELEASE-NOTES-11.4]]
- RELEASE-NOTES-6.2
- RELEASE-NOTES-9.1
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-12.2.md|RELEASE-NOTES-12.2]]
- RELEASE-NOTES-4.5
- RELEASE-NOTES-7.2
- RELEASE-NOTES-8.1
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.5.md|RELEASE-NOTES-11.5]]
- RELEASE-NOTES-9.0
- RELEASE-NOTES-6.3
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-10.4.md|RELEASE-NOTES-10.4]]
- RELEASE-NOTES-5.4
- RELEASE-NOTES-1.8
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.1.md|RELEASE-NOTES-11.1]]
- RELEASE-NOTES-8.5
- RELEASE-NOTES-4.1
- RELEASE-NOTES-5.0
- RELEASE-NOTES-10.0
- RELEASE-NOTES-6.7
- RELEASE-NOTES-9.4
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-2.0
- RELEASE-NOTES-3.1
- RELEASE-NOTES-1.6
- RELEASE-NOTES-2.1
- RELEASE-NOTES-3.0
- RELEASE-NOTES-2.5
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-2.6
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-4.6
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-12.1.md|RELEASE-NOTES-12.1]]
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.6.md|RELEASE-NOTES-11.6]]
- RELEASE-NOTES-7.1
- RELEASE-NOTES-8.2
- RELEASE-NOTES-9.3
- RELEASE-NOTES-6.0
- RELEASE-NOTES-7.5
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.2.md|RELEASE-NOTES-11.2]]
- RELEASE-NOTES-4.2
- RELEASE-NOTES-5.3
- RELEASE-NOTES-6.4
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-10.3.md|RELEASE-NOTES-10.3]]
- RELEASE-NOTES-7.4
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-11.3.md|RELEASE-NOTES-11.3]]
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-12.4.md|RELEASE-NOTES-12.4]]
- RELEASE-NOTES-4.3
- RELEASE-NOTES-5.2
- RELEASE-NOTES-6.5
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-10.2.md|RELEASE-NOTES-10.2]]
- [[domain-19-landscape-references/_archived-release-notes/observability/grafana/RELEASE-NOTES-12.0.md|RELEASE-NOTES-12.0]]
- RELEASE-NOTES-8.3
- RELEASE-NOTES-7.0
- RELEASE-NOTES-6.1
- RELEASE-NOTES-9.2
- [[entities/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference


<!-- risk-assessed -->
