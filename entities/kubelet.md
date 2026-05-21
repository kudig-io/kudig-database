---
title: kubelet
description: kubelet — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- kubelet
- node
- agent
- cri
- cgroups
- containerd
- cri-o
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 是什么
- 如何 kubelet
trigger_keywords:
- kubelet
prerequisites:
- kubectl-basics
---

# kubelet

## Role

kubelet runs on every worker node and is responsible for:
- Watching API Server for Pod assignments
- Managing container lifecycle via CRI (containerd/CRI-O)
- Mounting volumes via CSI
- Running health probes (liveness, readiness, startup)
- Reporting node and Pod status
- Evicting Pods under resource pressure

## Key Subsystems

| Subsystem | Function |
|-----------|----------|
| **PLEG** (Pod Lifecycle Event Generator) | Monitors container runtime, generates state change events that trigger syncPod |
| **Probe Manager** | Runs liveness, readiness, and startup probes |
| **Volume Manager** | Mounts/unmounts volumes, interacts with CSI drivers |
| **Eviction Manager** | Monitors node resources, evicts Pods when thresholds crossed |
| **cAdvisor** | Collects container resource metrics (CPU, memory, network, disk I/O) |
| **Status Manager** | Reports Pod and Node status to API Server |

## CRI (Container Runtime Interface)

kubelet communicates with container runtimes via gRPC-based CRI:
- `RunPodSandbox`: Create Pod network namespace
- `CreateContainer` / `StartContainer`: Container lifecycle
- `PullImage`: Pull container images
- `ListImages` / `RemoveImage`: Image management

## Key Configuration

| Parameter | Purpose | Recommended |
|-----------|---------|-------------|
| `--container-runtime-endpoint` | CRI socket | unix:///run/containerd/containerd.sock |
| `--cgroup-driver` | cgroup driver | systemd (must match runtime) |
| `--max-pods` | Max Pods per node | 110 (default), 500+ in cloud |
| `--eviction-hard` | Hard eviction threshold | memory.available<100Mi |
| `--pod-infra-container-image` | pause container image | registry.k8s.io/pause:3.9 |

## Certificate Rotation

kubelet auto-rotates its client certificate (`--rotate-certificates`), preventing certificate expiration issues.

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[entities/container-runtime.md|container-runtime]] — Container Runtime
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/resource-management.md|Resource Management]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[entities/container-runtime.md|Container Runtime]]

- [[domain-01-cluster-fundamentals/15-kubelet-deep-dive.md|15-kubelet-deep-dive]]
- [[domain-01-cluster-fundamentals/33-kubelet-eviction-thresholds.md|33-kubelet-eviction-thresholds]]
- [[domain-02-workloads-applications/20-kubelet-configuration.md|20-kubelet-configuration]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting.md|01-kubelet-troubleshooting]]
- [[domain-19-landscape-references/sandbox/virtual-kubelet/virtual-kubelet.md|virtual-kubelet]]
- [[skills/node-fta|Node 异常故障树分析]] — Cross-reference
- [[skills/deployment-fta|Deployment 异常故障树分析]] — Cross-reference
- [[skills/statefulset-fta|StatefulSet 异常故障树分析]] — Cross-reference
