---
title: Pod Lifecycle (concepts)
description: '- [[synthesis/Pod 生命周期 × Secret 管理|Pod 生命周期 × Secret 管理]] — 综合'
category: concepts
tags:
- k8s
- pod
- lifecycle
- containers
- probes
- kubelet
- statefulset
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Lifecycle 是什么
- 如何 Pod Lifecycle
trigger_keywords:
- Pod
- Lifecycle
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Pod Lifecycle

## State Machine

[[Pods|Pods]] transition through these phases:

| Phase | Meaning |
|-------|---------|
| **Pending** | Pod accepted by API Server but not yet running (waiting for scheduling, image pull, or volume mount) |
| **Running** | At least one container is running |
| **Succeeded** | All containers exited successfully (code 0) |
| **Failed** | At least one container exited with non-zero code |
| **Unknown** | Pod state cannot be determined (usually node communication failure) |

## Conditions

Each Pod has four condition types that describe its internal state:
- **PodScheduled**: Pod assigned to a node
- **Initialized**: All [[Init Containers|init containers]] completed
- **ContainersReady**: All containers passed readiness
- **Ready**: Pod can accept traffic (subset of ContainersReady + network ready)

## Container Startup Sequence

1. **Init Containers** run sequentially (each must succeed before next starts)
2. **Main Containers** start in parallel after all init containers complete
3. **[[Sidecar Containers|Sidecar Containers]]** (v1.28+) can run alongside init containers in parallel

## Health Probes

| Probe | Purpose | Trigger | Impact |
|-------|---------|---------|--------|
| **startupProbe** | Allow slow startup | Runs until first success | Disables other probes during startup |
| **livenessProbe** | Detect deadlocked/stuck processes | Periodic check | Container restart |
| **readinessProbe** | Determine if container can accept traffic | Periodic check | Remove from [[Service|Service]] endpoints |

Each probe can use HTTP GET, TCP Socket, or Exec commands.

## Termination Flow

1. Pod marked for deletion
2. **PreStop hook** executes (if defined)
3. **SIGTERM** sent to all containers
4. Wait for `terminationGracePeriodSeconds` (default 30s)
5. **SIGKILL** sent if still running
6. Resources cleaned up by [[kubelet|kubelet]]

## Related
- [[synthesis/Operator 模式 × Pod 生命周期|Operator 模式 × Pod 生命周期]] — 综合
- [[synthesis/Pod 生命周期 × Secret 管理|Pod 生命周期 × Secret 管理]] — 综合
- [[synthesis/Pod 生命周期 × 存储模型|Pod 生命周期 × 存储模型]] — 综合

- [[skills/troubleshoot-pod-issues|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[concepts/node-lifecycle-management|node-lifecycle-management]] — 节点生命周期管理
- [[entities/kubelet|kubelet]] — kubelet
- [[concepts/high-availability-patterns|high-availability-patterns]] — High Availability Patterns
- [[skills/configure-health-probes|configure-health-probes]] — Configure Health Probes
- [[deployment|Deployment]]
- [[entities/statefulset|StatefulSet]]
- [[concepts/high-availability-patterns|High Availability Patterns]]
- [[skills/configure-health-probes|Configure Health Probes]]
- [[entities/kubelet|kubelet]]

- Pod 生命周期事件表
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/k8s-workloads-domain-guide|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[references/workloads-terms|K8s 工作负载术语参考]] — Cross-reference
- [[references/k8s-architecture-domain-guide|Kubernetes Architecture Domain Guide]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- [[concepts/scheduling-algorithm|Scheduling Algorithm]] — Cross-reference
- [[skills/learn-inner-training|Kubernetes 培训：Inner Training]] — Cross-reference
- [[skills/kubelet-eviction-mechanism|kubelet 资源驱逐机制]] — Cross-reference
- [[skills/learn-public-training|Kubernetes 培训：Public Training]] — Cross-reference
- [[entities/inspektor-gadget|Inspektor Gadget]] — Cross-reference
- [[entities/container-runtime|Container Runtime]] — Cross-reference
- [[entities/clusterpedia|Clusterpedia]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pod-index|Pod 知识图谱索引]]
