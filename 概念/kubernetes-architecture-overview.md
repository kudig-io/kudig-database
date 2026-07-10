---
title: Kubernetes Architecture Overview
description: '- Kubernetes 架构全景图'
summary: 'Kubernetes follows a layered architecture with seven distinct layers:'
category: concepts
tags:
- k8s
- architecture
- control-plane
- data-plane
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Architecture Overview 是什么
- 如何 Kubernetes Architecture Overview
trigger_keywords:
- Kubernetes
- Architecture
- Overview
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[Kubernetes|Kubernetes]] Architecture Overview

## Layered Architecture

Kubernetes follows a layered architecture with seven distinct layers:

| Layer | Name | Responsibility | Key Components |
|-------|------|----------------|----------------|
| Layer 1 | Orchestration | Scheduling, automation | Scheduler, Controllers |
| Layer 2 | API | Unified entry, auth, admission | API Server, Admission |
| Layer 3 | Data | Persistent state | etcd |
| Layer 4 | Runtime | Container execution | [[kubelet|kubelet]], Container Runtime |
| Layer 5 | Network | Pod networking, load balancing | CNI, kube-proxy |
| Layer 6 | Storage | Persistent volume management | CSI, Volume Plugin |
| Layer 7 | Extension | Custom functionality | CRD, Operator, Webhook |

## Control Plane vs Data Plane

The **control plane** manages cluster state through four core components:
- **kube-apiserver**: Central REST API gateway, handles authentication, authorization, admission control, and persistence to [[etcd|etcd]]
- **etcd**: Distributed key-value store using Raft consensus and MVCC for state persistence
- **kube-scheduler**: Assigns Pods to nodes through a two-phase Filter+Score scheduling algorithm
- **kube-controller-manager**: Runs 40+ built-in controllers maintaining desired state via [[概念/controller-pattern.md|Controller Pattern]]

The **data plane** executes workloads on each node:
- **kubelet**: Node agent managing Pod lifecycle, communicates with API Server via Watch mechanism
- **kube-proxy**: Network proxy implementing Service load balancing (iptables/IPVS/eBPF)
- **Container Runtime**: Executes containers via CRI (containerd or CRI-O since v1.24)

## Communication Pattern

All components communicate exclusively through the API Server -- no direct component-to-component calls. This loose coupling enables independent upgrades and fault isolation. The Watch mechanism (HTTP chunked streaming based on etcd revisions) provides real-time state synchronization.

## Design Principles

Kubernetes is built on core principles: declarative API, controller reconciliation loop, loose coupling, extensibility (CRI/CNI/CSI/Device Plugin), self-healing, horizontal scaling, immutable infrastructure, and eventual consistency.

## Related

- [[pod-lifecycle]] — Pod Lifecycle
- [[概念/declarative-api.md|declarative-api]] — Declarative API
- [[实体/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[实体/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[etcd]] — etcd
- [[概念/controller-pattern.md|Controller Pattern]]
- [[概念/declarative-api.md|Declarative API]]
- [[概念/watch-mechanism.md|Watch Mechanism]]
- [[etcd|etcd]]
- [[实体/kube-apiserver.md|kube-apiserver]]
- [[实体/kube-scheduler.md|kube-scheduler]]
- [[实体/kubelet.md|kubelet]]
- [[概念/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]

- Kubernetes 架构全景图
- [[实体/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference
- [[实体/metal3-io.md|Metal3]] — Cross-reference
- [[实体/clusterpedia.md|Clusterpedia]] — Cross-reference


<!-- risk-assessed -->
