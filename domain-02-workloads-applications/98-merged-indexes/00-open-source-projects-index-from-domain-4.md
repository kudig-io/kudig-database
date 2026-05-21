---
title: Domain-4 工作负载 — 开源项目索引
description: '| **Descheduler** | Pod 重调度优化 | K8s SIG | v0.32.0 | 4k+ | Apache-2.0 |'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- scheduler
- vpa
- daemonset
- job
- cronjob
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-4 工作负载 — 开源项目索引 是什么
- 如何 Domain-4 工作负载 — 开源项目索引
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- Domain-4
- 工作负载
- 开源项目索引
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
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
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

# Domain-4 工作负载 — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes** | 工作负载编排核心 (Deployment/StatefulSet/DaemonSet/Job/CronJob) | Graduated | v1.33.0 | 115k+ | Apache-2.0 |
| **OpenKruise** | 高级工作负载 (CloneSet/Advanced StatefulSet/SidecarSet) | Incubating | v1.8.0 | 4.5k+ | Apache-2.0 |
| **KubeVirt** | VM 作为 K8s 工作负载 | Incubating | v1.5.0 | 5.5k+ | Apache-2.0 |
| **KEDA** | 事件驱动自动伸缩 | Graduated | v2.17.0 | 8.5k+ | Apache-2.0 |
| **VPA** | 垂直 Pod 自动伸缩 | K8s SIG | v1.3.0 | 5.5k+ | Apache-2.0 |
| **Descheduler** | Pod 重调度优化 | K8s SIG | v0.32.0 | 4k+ | Apache-2.0 |
| **Volcano** | 批处理调度 (Gang Scheduling) | 非 CNCF | v1.11.0 | 4k+ | Apache-2.0 |
| **Kueue** | 作业队列管理 | K8s SIG | v0.11.0 | 1.5k+ | Apache-2.0 |
| **Koordinator** | QoS 调度与混部 | 非 CNCF | v1.6.0 | 1.5k+ | Apache-2.0 |
| **Neko** | K8s 多容器调度框架 | 非 CNCF | v3.0.0 | 2k+ | Apache-2.0 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s 工作负载文档](https://kubernetes.io/docs/concepts/workloads/)
- [OpenKruise 文档](https://openkruise.io/docs/)
- [KEDA 文档](https://keda.sh/docs/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-02-workloads-applications/MOC.md|domain-02-workloads-applications MOC]]
- [[domain-02-workloads-applications/README.md|Domain-4: Kubernetes工作负载管理]]
- [[domain-02-workloads-applications/01-workload-overview-architecture.md|01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)]]
- [[domain-02-workloads-applications/02-deployment-production-patterns.md|02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)]]
- [[domain-02-workloads-applications/03-statefulset-advanced-operations.md|03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)]]
- [[domain-02-workloads-applications/04-daemonset-management.md|04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)]]
- [[domain-02-workloads-applications/05-job-cronjob-advanced.md|05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)]]
- [[domain-02-workloads-applications/06-workload-monitoring-alerting.md|06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)]]
- [[domain-02-workloads-applications/07-workload-troubleshooting-handbook.md|07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...]]
- [[domain-02-workloads-applications/08-multi-cloud-workload-strategy.md|08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...]]
- [[domain-02-workloads-applications/09-edge-computing-deployment.md|09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...]]
- [[domain-02-workloads-applications/10-workload-controllers-overview.md|工作负载控制器详解]]
