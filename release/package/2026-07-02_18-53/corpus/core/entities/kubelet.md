---
title: kubelet
description: kubelet — Kubernetes 生产运维知识库
summary: 'kubelet runs on every worker node and is responsible for:'
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kubelet

## Role

kubelet runs on every worker node and is responsible for:
- Watching API Server for Pod assignments
- Managing container lifecycle via CRI ([[containerd|containerd]]/CRI-O)
- Mounting volumes via CSI
- Running health probes (liveness, readiness, startup)
- Reporting node and Pod status
- Evicting [[Pods|Pods]] under resource pressure

## Key Subsystems

| Subsystem | Function |
|-----------|----------|
| **PLEG** (Pod Lifecycle Event Generator) | Monitors [[concepts/container-runtime.md|container runtime]], generates state change events that trigger syncPod |
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

- 15-kubelet-deep-dive
- 33-kubelet-eviction-thresholds
- 20-kubelet-configuration
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting.md|01-kubelet-troubleshooting]]
- virtual-kubelet
- [[skills/node-fta.md|Node 异常故障树分析]] — Cross-reference
- [[skills/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[skills/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
