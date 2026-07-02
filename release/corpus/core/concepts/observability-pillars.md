---
title: Observability Pillars
description: Observability Pillars — Kubernetes 生产运维知识库
summary: Observability Pillars — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- observability
- metrics
- logging
- tracing
- prometheus
- golden-signals
- etcd
- apiserver
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Observability Pillars 是什么
- 如何 Observability Pillars
trigger_keywords:
- Observability
- Pillars
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
---



# Observability Pillars

## Three Pillars

### Metrics

Numeric time-series data sampled at regular intervals.

**Golden Signals** (Google SRE methodology):
- **Latency**: Time to serve requests (p50, p95, p99)
- **Traffic**: Request rate per second
- **Errors**: Error rate (4xx, 5xx, failed health checks)
- **Saturation**: Resource utilization (CPU, memory, disk, network)

**Key Kubernetes metrics to monitor**:
- API Server: `apiserver_request_duration_seconds`, `apiserver_request_total`
- [[etcd|etcd]]: `etcd_disk_backend_commit_duration_seconds`, `etcd_mvcc_db_total_size_in_bytes`
- Scheduler: `scheduler_scheduling_attempt_duration_seconds`, `scheduler_pending_pods`
- [[kubelet|kubelet]]: `kubelet_pod_start_duration_seconds`, `kubelet_running_pods`

**Prometheus** is the de facto metrics collector, with Grafana for visualization.

### Logging

Structured and unstructured text output from containers and system components.

**Stacks**:
- **EFK**: Elasticsearch + Fluentd/Fluent Bit + Kibana
- **Loki**: Grafana Loki (label-based, Prometheus-style for logs)
- Centralized log aggregation with label-based querying

**Key log sources**: kube-apiserver, kubelet, controller-manager, scheduler, etcd, application containers.

### Distributed Tracing

Request-level trace across microservices.

**Components**:
- **Trace**: End-to-end request journey
- **Span**: Individual operation within a trace
- **Context Propagation**: Trace headers passed between services

**Tools**: Jaeger, Zipkin, OpenTelemetry (unified standard for traces, metrics, and logs).

## SLO/SLI Framework

- **SLI** (Service Level Indicator): What you measure (e.g., 99.9% of requests succeed)
- **SLO** (Service Level Objective): Target you aim for (e.g., 99.9% availability)
- **SLA** (Service Level Agreement): Contract with consequences for missing SLO
- **Error Budget**: 100% - SLO; the room for failure before breaching SLA

## Related
- [[concepts/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合
- [[concepts/控制器模式 × 可观测性.md|控制器模式 × 可观测性]] — 综合

- [[opentelemetry]] — OpenTelemetry
- [[fluentd]] — Fluentd
- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]

- [[concepts/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]
- [[entities/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference
