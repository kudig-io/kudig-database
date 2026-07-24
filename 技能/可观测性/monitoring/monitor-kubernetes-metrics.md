---
title: Monitor Kubernetes Metrics
description: Monitor Kubernetes Metrics — Kubernetes 生产运维知识库
summary: Monitor Kubernetes Metrics — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

## 生产案例

### 案例 1: metrics-server 证书过期导致 kubectl top 失败

| 时间 | 事件 |
|------|------|
| - | `kubectl top nodes` 报错: "error: metrics not available yet" |
| - | metrics-server Pod 日志: x509 certificate expired |
| - | 🟡 重新部署 metrics-server 或更新证书 |

**根因**: metrics-server 的 TLS 证书过期，无法从 kubelet 获取指标。

### 案例 2: 自定义指标 API 不可用导致 HPA 失效

**现象**: HPA 基于自定义指标(QPS)扩容，但指标获取失败。

**诊断**: custom-metrics-apiserver Pod CrashLoopBackOff

**修复**: 🟡 修复 custom-metrics-apiserver，HPA 恢复

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 指标系统影响 HPA | 检查 metrics-server |
| P1 | 部分指标缺失 | 检查 ServiceMonitor |
| P2 | 指标优化 | 调整采集间隔 |

## 面试要点

1. **Q: Kubernetes 指标 API 的层次？**
   A: ① metrics.k8s.io(资源指标: CPU/Memory，由 metrics-server 提供) ② custom.metrics.k8s.io(自定义 Pod 指标) ③ external.metrics.k8s.io(外部指标)。HPA 通过这三个 API 获取指标。

2. **Q: metrics-server 的工作原理？**
   A: metrics-server 从所有节点的 kubelet /metrics/resource 端点拉取 CPU/Memory 数据，聚合后通过 metrics.k8s.io API 暴露。不存储历史数据，仅供实时查询。

3. **Q: 生产环境关键指标清单？**
   A: ① 节点: CPU/Memory/Disk/Network ② Pod: 重启次数/OOM/资源使用率 ③ 控制平面: API 延迟/etcd 延迟/调度延迟 ④ 业务: QPS/错误率/P99 延迟。

## Related

- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/observability-pillars.md|observability-pillars]] — Observability Pillars
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[概念/observability-pillars.md|Observability Pillars]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[技能/控制面/etcd/backup-restore-etcd.md|Backup and Restore etcd]]


<!-- risk-assessed -->
