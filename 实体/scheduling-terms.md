---
title: K8s 调度术语参考
description: '# K8s 调度术语参考'
summary: '本页汇总了 **调度** 领域的 16 个 Kubernetes 术语定义与概念说明。'
category: references
tags:
- k8s
- dictionary
- scheduling
- kubelet
- scheduler
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 调度术语参考 是什么
- 如何 K8s 调度术语参考
trigger_keywords:
- K8s
- 调度术语参考
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 调度术语参考

本页汇总了 **调度** 领域的 16 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[实体/k8s-workload-management.md|k8s-workload-management]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **API-initiated Eviction** | Api Initiated Eviction | API 发起驱逐（API-initiated Eviction）是通过 Eviction API 创建 `Eviction` 对象来触发 Pod 优雅终止的过程 |
| **Assigning Pods to Nodes** | Assigning Pods To Nodes | Kubernetes 提供了多种方式将 Pod 约束到特定节点运行，或让 Pod 优先在某些节点上运行 |
| **Dynamic Resource Allocation** | Dynamic Resource Allocation | 动态资源分配（Dynamic Resource Allocation，DRA）是 Kubernetes v1 |
| **Gang Scheduling** | Gang Scheduling | Gang Scheduling（组调度）确保一组 Pod 以"全有或全无"的方式进行调度 |
| **Karpenter 自动扩缩容** | Karpenter Autoscaling | **Karpenter** 是 AWS 开源的 Kubernetes 节点自动扩缩容项目，已成为 2026 年替代传统 **Cluster Autosca... |
| **Kubernetes Scheduler** | Kubernetes Scheduler | 在 Kubernetes 中，调度（Scheduling）是指将 Pod 与节点（Node）进行匹配，以便 Kubelet 能够运行它们的过程 |
| **Node Declared Features** | Node Declared Features | 节点声明特性（Node Declared Features）是 Kubernetes v1 |
| **Node-pressure Eviction** | Node Pressure Eviction | 节点压力驱逐（Node-pressure Eviction）是 kubelet 主动终止 Pod 以回收节点资源的过程 |
| **Pod Overhead** | Pod Overhead | Pod Overhead（Pod 开销）是 Kubernetes 中一种用于核算 Pod 基础设施所消耗系统资源的方式 |
| **Pod Priority and Preemption** | Pod Priority And Preemption | Pod 优先级和抢占（Pod Priority and Preemption）是 Kubernetes v1 |
| **Pod Scheduling Readiness** | Pod Scheduling Readiness | Pod 调度就绪性（Pod Scheduling Readiness）允许用户通过设置或移除 Pod 的 ` |
| **Pod Topology Spread Constraints** | Pod Topology Spread Constraints | Pod 拓扑分布约束（Pod Topology Spread Constraints）用于控制 Pod 在集群中的分布方式，使其跨故障域（如区域、可用区、... |
| **Resource Bin Packing** | Resource Bin Packing | 资源装箱（Resource Bin Packing）是 kube-scheduler 中 `NodeResourcesFit` 插件的两种评分策略，用于提... |
| **Scheduler Performance Tuning** | Scheduler Performance Tuning | kube-scheduler 是 Kubernetes 的默认调度器，负责将 Pod 放置到集群的节点上 |
| **Scheduling Framework** | Scheduling Framework | 调度框架（Scheduling Framework）是 Kubernetes 调度器的可插拔架构 |
| **Taints and Tolerations** | Taints And Tolerations | 节点亲和性（Node affinity）是 Pod 的属性，用于将 Pod 吸引到一组节点（作为偏好或硬性要求） |

---

### API-initiated Eviction

API 发起驱逐（API-initiated Eviction）是通过 Eviction API 创建 `Eviction` 对象来触发 Pod 优雅终止的过程。可以直接调用 Eviction API，也可以通过 `kubectl drain` 等工具间接调用。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/api-initiated-eviction.md`）*

---

### Assigning Pods to Nodes

Kubernetes 提供了多种方式将 Pod 约束到特定节点运行，或让 Pod 优先在某些节点上运行。推荐的方法都使用标签选择器（label selectors）来促进选择。虽然通常不需要设置此类约束（调度器会自动进行合理放置），但在某些情况下，用户可能需要控制 Pod 部署到哪个节点。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/assigning-pods-to-nodes.md`）*

---

### Dynamic Resource Allocation

动态资源分配（Dynamic Resource Allocation，DRA）是 Kubernetes v1.35 中达到 stable 的特性。它允许用户在 Pod 之间请求和共享资源，这些资源通常是附加设备，如硬件加速器。DRA 提供了比 Device Plugin 更灵活的设备分类、请求和使用方式。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/dynamic-resource-allocation.md`）*

---

### Gang Scheduling

Gang Scheduling（组调度）确保一组 Pod 以"全有或全无"的方式进行调度。如果集群无法容纳整个组（或定义的最低数量），则组中没有任何 Pod 会被绑定到节点上。该特性在 Kubernetes v1.35 中为 alpha 状态。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/gang-scheduling.md`）*

---

### Karpenter 自动扩缩容

**Karpenter** 是 AWS 开源的 Kubernetes 节点自动扩缩容项目，已成为 2026 年替代传统 **Cluster Autoscaler** 的主流方案。与 Cluster Autoscaler 相比，Karpenter 不再依赖预配置的节点组（Node Group/Auto Scaling Group），而是直接观察 Pending Pod 的资源需求，实时选择最优的实例类型和购买选项（On-Demand / Spot），并在秒级内启动新节点。这种"**直接调度到云**"的架构显著提升了资源利用率、降低了成本和启动延迟。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/karpenter-autoscaling.md`）*

---

### Kubernetes Scheduler

在 Kubernetes 中，调度（Scheduling）是指将 Pod 与节点（Node）进行匹配，以便 Kubelet 能够运行它们的过程。kube-scheduler 是 Kubernetes 的默认调度器，作为控制平面的一部分运行。它负责为新创建的或未调度的 Pod 选择最优的节点。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/kubernetes-scheduler.md`）*

---

### Node Declared Features

节点声明特性（Node Declared Features）是 Kubernetes v1.35 中引入的 alpha 特性。Kubernetes 节点使用声明特性来报告特定新特性或特性门控功能的可用性。控制平面组件利用这些信息做出更好的决策。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/node-declared-features.md`）*

---

### Node-pressure Eviction

节点压力驱逐（Node-pressure Eviction）是 kubelet 主动终止 Pod 以回收节点资源的过程。kubelet 监控节点的内存、磁盘空间、文件系统 inode 和 PID 等资源，当某些资源达到特定消耗水平时，kubelet 会主动使一个或多个 Pod 失败来回收资源，防止饥饿。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/node-pressure-eviction.md`）*

---

### Pod Overhead

Pod Overhead（Pod 开销）是 Kubernetes 中一种用于核算 Pod 基础设施所消耗系统资源的方式。这些资源是容器内部运行所需资源之外的额外开销。Pod 的开销在准入时根据 Pod 的 RuntimeClass 相关联的开销进行设置。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/pod-overhead.md`）*

---

### Pod Priority and Preemption

Pod 优先级和抢占（Pod Priority and Preemption）是 Kubernetes v1.14 中达到 stable 的特性。Pod 可以具有优先级，表示该 Pod 相对于其他 Pod 的重要性。如果某个 Pod 无法被调度，调度器会尝试抢占（驱逐）优先级较低的 Pod，以使该 pending Pod 能够被调度。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/pod-priority-and-preemption.md`）*

---

### Pod Scheduling Readiness

Pod 调度就绪性（Pod Scheduling Readiness）允许用户通过设置或移除 Pod 的 `.spec.schedulingGates` 字段来控制 Pod 何时准备好被调度器考虑。在 Kubernetes v1.30 中达到 stable 状态。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/pod-scheduling-readiness.md`）*

---

### Pod Topology Spread Constraints

Pod 拓扑分布约束（Pod Topology Spread Constraints）用于控制 Pod 在集群中的分布方式，使其跨故障域（如区域、可用区、节点）或其他用户定义的拓扑域均匀分布。这有助于实现高可用性和高效的资源利用。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/pod-topology-spread-constraints.md`）*

---

### Resource Bin Packing

资源装箱（Resource Bin Packing）是 kube-scheduler 中 `NodeResourcesFit` 插件的两种评分策略，用于提高集群资源利用率。这两种策略分别是 `MostAllocated` 和 `RequestedToCapacityRatio`。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/resource-bin-packing.md`）*

---

### Scheduler Performance Tuning

kube-scheduler 是 Kubernetes 的默认调度器，负责将 Pod 放置到集群的节点上。在大型集群中，可以通过调整调度器的行为来平衡调度延迟（新 Pod 快速放置）和准确性（调度器很少做出差的放置决策）。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/scheduler-performance-tuning.md`）*

---

### Scheduling Framework

调度框架（Scheduling Framework）是 Kubernetes 调度器的可插拔架构。它由一组直接编译到调度器中的"插件"API 组成。这些 API 允许将大多数调度功能实现为插件，同时保持调度核心轻量且可维护。该功能在 Kubernetes v1.19 中达到 stable 状态。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/scheduling-framework.md`）*

---

### Taints and Tolerations

节点亲和性（Node affinity）是 Pod 的属性，用于将 Pod 吸引到一组节点（作为偏好或硬性要求）。而污点（Taints）正好相反——它们允许节点排斥一组 Pod。容忍度（Tolerations）应用于 Pod，允许调度器调度具有匹配污点的 Pod。

污点和容忍度协同工作，确保 Pod 不会被调度到不合适的节点上。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/scheduling/taints-and-tolerations.md`）*

---

## 相关页面

- [[实体/k8s-workload-management.md|k8s-workload-management]]

## 来源文件

- `系统基础/topic-dictionary/scheduling/api-initiated-eviction.md`
- `系统基础/topic-dictionary/scheduling/assigning-pods-to-nodes.md`
- `系统基础/topic-dictionary/scheduling/dynamic-resource-allocation.md`
- `系统基础/topic-dictionary/scheduling/gang-scheduling.md`
- `系统基础/topic-dictionary/scheduling/karpenter-autoscaling.md`
- `系统基础/topic-dictionary/scheduling/kubernetes-scheduler.md`
- `系统基础/topic-dictionary/scheduling/node-declared-features.md`
- `系统基础/topic-dictionary/scheduling/node-pressure-eviction.md`
- `系统基础/topic-dictionary/scheduling/pod-overhead.md`
- `系统基础/topic-dictionary/scheduling/pod-priority-and-preemption.md`
- `系统基础/topic-dictionary/scheduling/pod-scheduling-readiness.md`
- `系统基础/topic-dictionary/scheduling/pod-topology-spread-constraints.md`
- `系统基础/topic-dictionary/scheduling/resource-bin-packing.md`
- `系统基础/topic-dictionary/scheduling/scheduler-performance-tuning.md`
- `系统基础/topic-dictionary/scheduling/scheduling-framework.md`
- `系统基础/topic-dictionary/scheduling/taints-and-tolerations.md`

## Related

- [[实体/observability-terms.md|observability-terms]] — K8s 可观测性术语参考
- [[实体/storage-terms.md|storage-terms]] — K8s 存储术语参考
- [[实体/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
