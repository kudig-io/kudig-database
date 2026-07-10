---
title: Kubernetes Architecture Domain Guide
description: Kubernetes Architecture Domain Guide — Kubernetes 生产运维知识库
summary: Kubernetes Architecture Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- architecture
- 集群基础
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Architecture Domain Guide 是什么
- 如何 Kubernetes Architecture Domain Guide
trigger_keywords:
- Kubernetes
- Architecture
- Domain
- Guide
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Architecture Domain Guide

## Source

Distilled from 集群基础 (25 documents, Kubernetes v1.29-v1.33).

## Layered Architecture

| Layer | Name | Components |
|-------|------|-----------|
| Layer 1 | Orchestration | Scheduler, Controllers |
| Layer 2 | API | API Server, Admission |
| Layer 3 | Data | etcd |
| Layer 4 | Runtime | kubelet, Container Runtime |
| Layer 5 | Network | CNI, kube-proxy |
| Layer 6 | Storage | CSI, Volume Plugin |
| Layer 7 | Extension | CRD, Operator, Webhook |

## Control Plane Components

- **kube-apiserver**: Port 6443 HTTPS. Stateless. Authentication + Authorization + Admission + etcd persistence.
- **etcd**: Ports 2379/2380. Raft consensus, MVCC storage. 3-node (tolerates 1 failure) or 5-node (tolerates 2).
- **kube-scheduler**: Port 10259. Filter+Score algorithm, leader election for HA.
- **kube-controller-manager**: Port 10257. 40+ controllers, leader election for HA.
- **cloud-controller-manager**: Port 10258. Cloud-specific Node/Service/Route controllers.

## Node Components

- **kubelet**: Port 10250. Pod lifecycle, CRI, CSI, probes, eviction. Max 110 Pods default.
- **kube-proxy**: Port 10249. iptables/IPVS/eBPF Service load balancing.
- **Container Runtime**: CRI via Unix socket (containerd or CRI-O).

## HA Sizing

| Cluster Size | Nodes | Pods | Master Config | etcd Config |
|-------------|-------|------|---------------|-------------|
| Small | <50 | <1500 | 2C4G | 3-node, 2C4G, SSD |
| Medium | 50-250 | 1500-7500 | 4C8G | 3-node, 4C8G, SSD |
| Large | 250-1000 | 7500-30000 | 8C16G | 5-node, 8C16G, NVMe |
| XLarge | >1000 | >30000 | 16C32G | 5-node, 16C32G, NVMe |

## Related

- [[reference|#reference Hub]] — tag hub

- [[概念/observability-pillars.md|observability-pillars]] — 01-observability-architecture-overview Pillars
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[概念/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[概念/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[概念/high-availability-patterns.md|High Availability Patterns]]
- [[概念/security-defense-depth.md|Defense-in-Depth Security]]
- [[概念/observability-pillars.md|Observability Pillars]]
- [[概念/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]

- 01-plane-architecture-overview
- [[平台工程/代码分析/cluster-cert/01-pki-architecture.md|01-pki-architecture]]

<!-- risk-assessed -->
