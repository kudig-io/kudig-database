---
title: Kubernetes Architecture Domain Guide
description: Kubernetes Architecture Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- architecture
- domain-01-cluster-fundamentals
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
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

# Kubernetes Architecture Domain Guide

## Source

Distilled from domain-01-cluster-fundamentals (25 documents, Kubernetes v1.29-v1.33).

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

- [[concepts/observability-pillars.md|observability-pillars]] — [[domain-06-observability/01-observability-architecture-overview.md|01-observability-architecture-overview]] Pillars
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[concepts/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[concepts/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]
- [[concepts/security-defense-depth.md|Defense-in-Depth Security]]
- [[concepts/observability-pillars.md|Observability Pillars]]
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[concepts/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]

- [[domain-01-cluster-fundamentals/01-plane-architecture-overview.md|01-plane-architecture-overview]]
- [[domain-07-platform-engineering/topic-code-analysis/cluster-cert/01-pki-architecture.md|01-pki-architecture]]