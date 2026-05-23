---
title: Monitor Kubernetes Metrics
description: Monitor Kubernetes Metrics — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- monitoring
- prometheus
- metrics
- alerting
- golden-signals
- etcd
- apiserver
- kubelet
- scheduler
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Monitor Kubernetes Metrics 是什么
- 如何 Monitor Kubernetes Metrics
trigger_keywords:
- Monitor
- Kubernetes
- Metrics
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

# Monitor Kubernetes Metrics

## Metrics Collection Stack

- **Prometheus**: Pull-based metrics collector with PromQL query language
- **Grafana**: Visualization dashboards
- **metrics-server**: Lightweight in-cluster metrics for HPA (`kubectl top`)
- **kube-state-metrics**: Exposes Kubernetes object state as metrics

## Critical Metrics to Monitor

### Control Plane

| Component | Key Metrics | Alert Threshold |
|-----------|------------|-----------------|
| **API Server** | `apiserver_request_duration_seconds`, `apiserver_request_total` | P99 latency > 4s |
| **etcd** | `etcd_disk_backend_commit_duration_seconds`, `etcd_server_has_leader` | fsync > 500ms, no leader |
| **Scheduler** | `scheduler_scheduling_attempt_duration_seconds`, `scheduler_pending_pods` | High pending [[Pods|pods]] |
| **Controller Manager** | `workqueue_depth`, `workqueue_queue_duration_seconds` | Growing queue depth |

### Nodes

| Component | Key Metrics | Alert Threshold |
|-----------|------------|-----------------|
| **[[kubelet|kubelet]]** | `kubelet_running_pods`, `kubelet_pod_start_duration_seconds` | NodeNotReady > 5min |
| **cAdvisor** | `container_cpu_usage_seconds_total`, `container_memory_working_set_bytes` | OOMKill events |
| **Node Exporter** | `node_cpu_seconds_total`, `node_memory_MemAvailable_bytes` | MemoryPressure |

## Alert Categories

| Severity | Example Alerts | Response |
|----------|---------------|----------|
| **Critical** | KubeAPIDown, EtcdNoLeader, NodeNotReady | Immediate page |
| **Warning** | APIServerLatencyHigh, PodCrashLooping, NodeMemoryPressure | Investigate within SLA |
| **Info** | High resource utilization, approaching quota | Plan capacity |

## Golden Signals

Apply the four golden signals to application monitoring:
1. **Latency**: Response time distribution (p50, p95, p99)
2. **Traffic**: Request rate
3. **Errors**: Error rate and types
4. **Saturation**: Resource utilization and capacity headroom

## Related

- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/observability-pillars|observability-pillars]] — Observability Pillars
- [[concepts/kubernetes-architecture-overview|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/observability-pillars|Observability Pillars]]
- [[concepts/kubernetes-architecture-overview|Kubernetes Architecture Overview]]
- [[skills/troubleshoot-pod-issues|Troubleshoot Pod Issues]]
- [[skills/backup-restore-etcd|Backup and Restore etcd]]
