---
title: Prometheus and Grafana
description: Prometheus and Grafana — Kubernetes 生产运维知识库
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

# Prometheus and Grafana

## Prometheus

Prometheus is the de facto standard monitoring system for Kubernetes, providing:

- **Pull-based architecture**: Scrapes metrics from targets via HTTP
- **Time-series database**: Local storage with configurable retention
- **PromQL**: Powerful query language for aggregation, filtering, and mathematical operations
- **Service Discovery**: Native K8s service discovery for automatic target detection
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
- [[synthesis/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合
- [[synthesis/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合
- [[synthesis/控制器模式 × 可观测性.md|控制器模式 × 可观测性]] — 综合
- [[synthesis/CRD × 可观测性.md|CRD × 可观测性]] — 综合
- [[synthesis/etcd × 可观测性|etcd × 可观测性]] — 综合
- [[synthesis/Operator 模式 × 可观测性|Operator 模式 × 可观测性]] — 综合

- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/production-operations-best-practices.md|production-operations-best-practices]] — Production Operations Best Practices
- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[concepts/production-operations-best-practices.md|Production Operations Best Practices]]
- [[concepts/microservice-resilience-patterns.md|Microservice Resilience Patterns]]

- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-06-observability/02-grafana-enterprise-observability.md|02-grafana-enterprise-observability]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.32.md|RELEASE-NOTES-2.32]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.22.md|RELEASE-NOTES-2.22]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.47.md|RELEASE-NOTES-2.47]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.16.md|RELEASE-NOTES-2.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.36.md|RELEASE-NOTES-2.36]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.53.md|RELEASE-NOTES-2.53]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.12.md|RELEASE-NOTES-2.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.43.md|RELEASE-NOTES-2.43]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.26.md|RELEASE-NOTES-2.26]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.37.md|RELEASE-NOTES-2.37]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.52.md|RELEASE-NOTES-2.52]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.13.md|RELEASE-NOTES-2.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.42.md|RELEASE-NOTES-2.42]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.27.md|RELEASE-NOTES-2.27]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.33.md|RELEASE-NOTES-2.33]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.23.md|RELEASE-NOTES-2.23]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.46.md|RELEASE-NOTES-2.46]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.17.md|RELEASE-NOTES-2.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.4.md|RELEASE-NOTES-2.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.38.md|RELEASE-NOTES-2.38]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.28.md|RELEASE-NOTES-2.28]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.18.md|RELEASE-NOTES-2.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.49.md|RELEASE-NOTES-2.49]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.19.md|RELEASE-NOTES-2.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.48.md|RELEASE-NOTES-2.48]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.5.md|RELEASE-NOTES-2.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.39.md|RELEASE-NOTES-2.39]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.29.md|RELEASE-NOTES-2.29]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.6.md|RELEASE-NOTES-2.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.7.md|RELEASE-NOTES-3.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.10.md|RELEASE-NOTES-3.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.11.md|RELEASE-NOTES-3.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.34.md|RELEASE-NOTES-2.34]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.51.md|RELEASE-NOTES-2.51]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.41.md|RELEASE-NOTES-2.41]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.10.md|RELEASE-NOTES-2.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.9.md|RELEASE-NOTES-3.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.24.md|RELEASE-NOTES-2.24]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.55.md|RELEASE-NOTES-2.55]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.30.md|RELEASE-NOTES-2.30]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.20.md|RELEASE-NOTES-2.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.14.md|RELEASE-NOTES-2.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.45.md|RELEASE-NOTES-2.45]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.54.md|RELEASE-NOTES-2.54]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.31.md|RELEASE-NOTES-2.31]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.21.md|RELEASE-NOTES-2.21]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.15.md|RELEASE-NOTES-2.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.44.md|RELEASE-NOTES-2.44]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.35.md|RELEASE-NOTES-2.35]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.9.md|RELEASE-NOTES-2.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.50.md|RELEASE-NOTES-2.50]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.40.md|RELEASE-NOTES-2.40]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.11.md|RELEASE-NOTES-2.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.8.md|RELEASE-NOTES-3.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.25.md|RELEASE-NOTES-2.25]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.0.md|RELEASE-NOTES-11.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-8.4.md|RELEASE-NOTES-8.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.0.md|RELEASE-NOTES-4.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-5.1.md|RELEASE-NOTES-5.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.1.md|RELEASE-NOTES-10.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-9.5.md|RELEASE-NOTES-9.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.6.md|RELEASE-NOTES-6.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.3.md|RELEASE-NOTES-12.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.4.md|RELEASE-NOTES-4.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-8.0.md|RELEASE-NOTES-8.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-7.3.md|RELEASE-NOTES-7.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.4.md|RELEASE-NOTES-11.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.2.md|RELEASE-NOTES-6.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-9.1.md|RELEASE-NOTES-9.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.2.md|RELEASE-NOTES-12.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.5.md|RELEASE-NOTES-4.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-7.2.md|RELEASE-NOTES-7.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-8.1.md|RELEASE-NOTES-8.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.5.md|RELEASE-NOTES-11.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-9.0.md|RELEASE-NOTES-9.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.3.md|RELEASE-NOTES-6.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.4.md|RELEASE-NOTES-10.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-5.4.md|RELEASE-NOTES-5.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.1.md|RELEASE-NOTES-11.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-8.5.md|RELEASE-NOTES-8.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.1.md|RELEASE-NOTES-4.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-5.0.md|RELEASE-NOTES-5.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.0.md|RELEASE-NOTES-10.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.7.md|RELEASE-NOTES-6.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-9.4.md|RELEASE-NOTES-9.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-2.5.md|RELEASE-NOTES-2.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-2.6.md|RELEASE-NOTES-2.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.6.md|RELEASE-NOTES-4.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.1.md|RELEASE-NOTES-12.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.6.md|RELEASE-NOTES-11.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-7.1.md|RELEASE-NOTES-7.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-8.2.md|RELEASE-NOTES-8.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-9.3.md|RELEASE-NOTES-9.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.0.md|RELEASE-NOTES-6.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-7.5.md|RELEASE-NOTES-7.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.2.md|RELEASE-NOTES-11.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.2.md|RELEASE-NOTES-4.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-5.3.md|RELEASE-NOTES-5.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.4.md|RELEASE-NOTES-6.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.3.md|RELEASE-NOTES-10.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-7.4.md|RELEASE-NOTES-7.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-11.3.md|RELEASE-NOTES-11.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.4.md|RELEASE-NOTES-12.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-4.3.md|RELEASE-NOTES-4.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-5.2.md|RELEASE-NOTES-5.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.5.md|RELEASE-NOTES-6.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-10.2.md|RELEASE-NOTES-10.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-12.0.md|RELEASE-NOTES-12.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-8.3.md|RELEASE-NOTES-8.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-7.0.md|RELEASE-NOTES-7.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-6.1.md|RELEASE-NOTES-6.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/grafana/RELEASE-NOTES-9.2.md|RELEASE-NOTES-9.2]]
- [[entities/inspektor-gadget|Inspektor Gadget]] — Cross-reference
