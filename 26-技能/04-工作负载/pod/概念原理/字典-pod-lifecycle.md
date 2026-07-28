---
title: Pod Lifecycle (concepts)
description: '- [[22-概念/11-交叉分析/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
summary: '- [[22-概念/11-交叉分析/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Lifecycle

## State Machine

[[pods|Pods]] transition through these phases:

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
- **Initialized**: All [[17-系统基础/06-知识字典/workloads/init-containers.md|init containers]] completed
- **ContainersReady**: All containers passed readiness
- **Ready**: Pod can accept traffic (subset of ContainersReady + network ready)

## Container Startup Sequence

1. **Init Containers** run sequentially (each must succeed before next starts)
2. **Main Containers** start in parallel after all init containers complete
3. **[[17-系统基础/06-知识字典/workloads/sidecar-containers.md|Sidecar Containers]]** (v1.28+) can run alongside init containers in parallel

## Health Probes

| Probe | Purpose | Trigger | Impact |
|-------|---------|---------|--------|
| **startupProbe** | Allow slow startup | Runs until first success | Disables other probes during startup |
| **livenessProbe** | Detect deadlocked/stuck processes | Periodic check | Container restart |
| **readinessProbe** | Determine if container can accept traffic | Periodic check | Remove from [[service\|Service]] endpoints |

Each probe can use HTTP GET, TCP Socket, or Exec commands.

## Termination Flow

1. Pod marked for deletion
2. **PreStop hook** executes (if defined)
3. **SIGTERM** sent to all containers
4. Wait for `terminationGracePeriodSeconds` (default 30s)
5. **SIGKILL** sent if still running
6. Resources cleaned up by [[kubelet|kubelet]]

## 参考链接

- [Pod Lifecycle]()

## Related
- [[22-概念/11-交叉分析/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合
- [[22-概念/11-交叉分析/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合
- [[22-概念/11-交叉分析/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]] — 综合

- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[22-概念/08-可靠性与运维/node-lifecycle-management.md|node-lifecycle-management]] — 节点生命周期管理
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[22-概念/08-可靠性与运维/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[26-技能/04-工作负载/pod/运维操作/configure-health-probes.md|configure-health-probes]] — Configure Health Probes
- [[17-系统基础/06-知识字典/workloads/deployment.md|Deployment]]
- [[23-实体/02-K8s核心组件/statefulset.md|StatefulSet]]
- [[22-概念/08-可靠性与运维/high-availability-patterns.md|High Availability Patterns]]
- [[26-技能/04-工作负载/pod/运维操作/configure-health-probes.md|Configure Health Probes]]
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]]

- Pod 生命周期事件表
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[23-实体/15-参考与索引/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[23-实体/15-参考与索引/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-architecture-domain-guide.md|Kubernetes Architecture Domain Guide]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- [[22-概念/07-调度与资源/scheduling-algorithm.md|Scheduling Algorithm]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-inner-training.md|Kubernetes 培训：Inner Training]] — Cross-reference
- [[26-技能/03-节点/node/运维操作/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-public-training.md|Kubernetes 培训：Public Training]] — Cross-reference
- [[23-实体/07-可观测性/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference
- [[23-实体/02-K8s核心组件/container-runtime.md|Container Runtime]] — Cross-reference
- [[23-实体/09-编排调度/clusterpedia.md|Clusterpedia]] — Cross-reference
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]


<!-- risk-assessed -->
