---
title: K8s 工作负载术语参考
description: '| **Advanced Pod Configuration** | Advanced Pod Configuration | 本页涵盖
  Pod 的高级配置主题，包括 PriorityClass、RuntimeClass、安全上下文（security context）以及影响 Po... |'
summary: '| **Advanced Pod Configuration** | Advanced Pod Configuration | 本页涵盖 Pod
  的高级配置主题，包括 PriorityClass、RuntimeClass、安全上下文（security context）以及影响 Po... |'
category: references
tags:
- k8s
- dictionary
- workloads
- etcd
- kubelet
- hpa
- vpa
- pdb
- statefulset
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 工作负载术语参考 是什么
- 如何 K8s 工作负载术语参考
trigger_keywords:
- K8s
- 工作负载术语参考
prerequisites:
- kubectl-basics
- pod-lifecycle
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 工作负载术语参考

本页汇总了 **工作负载** 领域的 33 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[实体/k8s-workloads-domain-guide.md|k8s-workloads-domain-guide]] | [[实体/k8s-workload-management.md|k8s-workload-management]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **Advanced Pod Configuration** | Advanced Pod Configuration | 本页涵盖 Pod 的高级配置主题，包括 PriorityClass、RuntimeClass、安全上下文（security context）以及影响 Po... |
| **Automatic Cleanup for Finished Jobs** | Automatic Cleanup For Finished Jobs | TTL-after-finished 控制器为已完成的 Job 对象提供基于生存时间（TTL）的自动清理机制 |
| **Autoscaling Workloads** | Autoscaling Workloads | 自动扩缩容（Autoscaling）允许工作负载根据资源需求自动调整规模，使集群能够更弹性和高效地响应变化 |
| **容器环境（Container Environment）** | Container Environment | Kubernetes 容器环境为容器提供了若干重要资源，包括文件系统、容器自身信息，以及集群中其他对象的信息 |
| **容器生命周期钩子（Container Lifecycle Hooks）** | Container Lifecycle Hooks | 类似于 Angular 等编程框架中的组件生命周期钩子，Kubernetes 为容器提供了生命周期钩子（Lifecycle Hooks）机制 |
| **容器运行时接口（Container Runtime Interface, CRI）** | Container Runtime Interface Cri | 容器运行时接口（CRI）是一个插件接口，它使 kubelet 能够使用多种不同的容器运行时，而无需重新编译集群组件 |
| **CronJob** | Cronjob | CronJob 用于按重复的时间表创建 Job，类似于 Unix 系统中的 crontab |
| **DaemonSet** | Daemonset | DaemonSet 确保所有（或部分）节点上都运行一个 Pod 副本 |
| **Deployments** | Deployments | Deployment 为 Pod 和 ReplicaSet 提供声明式更新能力 |
| **Disruptions** | Disruptions | 本页介绍影响 Pod 可用性的中断类型，以及如何通过 Pod Disruption Budget（PDB）等机制来管理自愿中断，帮助应用所有者和集群管理员... |
| **Downward API** | Downward Api | Downward API 允许容器在不使用 Kubernetes 客户端或访问 API Server 的情况下，消费关于自身或集群的信息 |
| **Ephemeral Containers** | Ephemeral Containers | Ephemeral（临时）容器是一种在现有 Pod 中临时运行的特殊容器，主要用于用户发起的故障排查操作（如调试），不适用于构建应用程序 |
| **Horizontal Pod Autoscaling** | Horizontal Pod Autoscaling | HorizontalPodAutoscaler（HPA）是 Kubernetes 的 API 资源和控制器，可根据观察到的指标（如 CPU 利用率、内存利... |
| **容器镜像（Images）** | Images | 容器镜像（Container Image）是封装了应用程序及其所有软件依赖项的二进制数据，是一个可独立运行的可执行软件包，并对其运行时环境做出非常明确的假设 |
| **Init Containers** | Init Containers | Init 容器是在 Pod 启动期间、于应用容器之前运行的特殊容器 |
| **Jobs** | Jobs | Job 用于表示一次性任务，运行到完成即停止 |
| **Managing Workloads** | Managing Workloads | 本页介绍在 Kubernetes 中部署应用后，如何使用各种工具和实践来管理、更新和扩展工作负载，涵盖 kubectl 批量操作、应用更新、金丝雀发布、资... |
| **Pod Group Policies** | Pod Group Policies | Pod Group Policies 是 Workload API 的组成部分（Alpha，v1 |
| **Pod Hostname** | Pod Hostname | 本页解释 Pod 主机名的设置方式、配置后的潜在副作用以及底层机制 |
| **Pod Lifecycle** | Pod Lifecycle | Pod 遵循一个确定的生命周期，从 `Pending` 阶段开始，如果至少一个主容器正常启动则进入 `Running`，最终根据容器终止情况进入 `Suc... |
| **Pod Quality of Service Classes** | Pod Quality Of Service Classes | Kubernetes 根据 Pod 内容器的资源请求（requests）和限制（limits）为每个 Pod 分配一个服务质量（QoS）等级 |
| **Pods** | Pods | Pod 是 Kubernetes 中最小的可部署计算单元，它是一组共享存储和网络资源、并协同运行的一个或多个容器的集合 |
| **ReplicaSet** | Replicaset | ReplicaSet 的作用是维护一组稳定运行的 Pod 副本 |
| **ReplicationController** | Replicationcontroller | ReplicationController 是一种遗留 API，用于确保指定数量的 Pod 副本始终处于运行状态 |
| **运行时类（RuntimeClass）** | Runtime Class | RuntimeClass 是 Kubernetes 中用于选择容器运行时配置的特性（自 v1 |
| **Sidecar Containers** | Sidecar Containers | Sidecar 容器是与主应用容器运行在同一 Pod 内的辅助容器，用于增强或扩展主应用功能，如日志收集、监控、安全代理或数据同步 |
| **Spot 与可抢占工作负载** | Spot And Preemptible Workloads | 在云原生环境中，**Spot 实例（AWS）、Preemptible VM（GCP）和 Low-priority VM（Azure）** 是云厂商以大幅折... |
| **StatefulSets** | Statefulsets | StatefulSet 是用于管理有状态应用的工作负载 API 对象 |
| **User Namespaces** | User Namespaces | 用户命名空间（User Namespaces）是 Linux 的一项特性，用于将容器内的用户与主机（节点）上的用户隔离 |
| **Vertical Pod Autoscaling** | Vertical Pod Autoscaling | VerticalPodAutoscaler（VPA）自动调整工作负载（如 Deployment、StatefulSet）中 Pod 的资源请求（reque... |
| **Workload API** | Workload Api | Workload API 是 Kubernetes v1 |
| **Workload Management** | Workload Management | Kubernetes 提供多个内置 API 用于声明式地管理工作负载及其组件 |
| **Workload Reference** | Workload Reference | Workload Reference 是 Kubernetes v1 |

---

### Advanced Pod Configuration

本页涵盖 Pod 的高级配置主题，包括 PriorityClass、RuntimeClass、安全上下文（security context）以及影响 Pod 调度的相关机制。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/advanced-pod-configuration.md`）*

---

### Automatic Cleanup for Finished Jobs

TTL-after-finished 控制器为已完成的 Job 对象提供基于生存时间（TTL）的自动清理机制。它有助于减少 API Server 中已完成 Job 的累积，降低 etcd 压力。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/automatic-cleanup-for-finished-jobs.md`）*

---

### Autoscaling Workloads

自动扩缩容（Autoscaling）允许工作负载根据资源需求自动调整规模，使集群能够更弹性和高效地响应变化。Kubernetes 支持水平扩缩容（增加/减少副本数）和垂直扩缩容（调整单个 Pod 的资源）。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/autoscaling-workloads.md`）*

---

### 容器环境（Container Environment）

Kubernetes 容器环境为容器提供了若干重要资源，包括文件系统、容器自身信息，以及集群中其他对象的信息。了解这些资源有助于开发人员在容器内正确获取运行时的上下文信息。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/container-environment.md`）*

---

### 容器生命周期钩子（Container Lifecycle Hooks）

类似于 Angular 等编程框架中的组件生命周期钩子，Kubernetes 为容器提供了生命周期钩子（Lifecycle Hooks）机制。该机制使容器能够感知自身管理生命周期中的事件，并在相应钩子触发时执行处理程序（handler）中的代码。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/container-lifecycle-hooks.md`）*

---

### 容器运行时接口（Container Runtime Interface, CRI）

容器运行时接口（CRI）是一个插件接口，它使 kubelet 能够使用多种不同的容器运行时，而无需重新编译集群组件。CRI 是 kubelet 与容器运行时之间的主要通信协议，采用 gRPC 定义。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/container-runtime-interface-cri.md`）*

---

### CronJob

CronJob 用于按重复的时间表创建 Job，类似于 Unix 系统中的 crontab。它适合执行定期任务，如数据备份、报表生成、定时清理等。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/cronjob.md`）*

---

### DaemonSet

DaemonSet 确保所有（或部分）节点上都运行一个 Pod 副本。当节点加入集群时，Pod 会被自动创建；当节点从集群移除时，Pod 会被垃圾回收。删除 DaemonSet 会清理其创建的所有 Pod。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/daemonset.md`）*

---

### Deployments

Deployment 为 Pod 和 ReplicaSet 提供声明式更新能力。用户描述期望状态，Deployment 控制器以受控速率将实际状态变更为期望状态。它是 Kubernetes 中管理无状态应用最常用的工作负载资源。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/deployments.md`）*

---

### Disruptions

本页介绍影响 Pod 可用性的中断类型，以及如何通过 Pod Disruption Budget（PDB）等机制来管理自愿中断，帮助应用所有者和集群管理员维护高可用性。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/disruptions.md`）*

---

### Downward API

Downward API 允许容器在不使用 Kubernetes 客户端或访问 API Server 的情况下，消费关于自身或集群的信息。它降低了应用与 Kubernetes 的耦合度。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/downward-api.md`）*

---

### Ephemeral Containers

Ephemeral（临时）容器是一种在现有 Pod 中临时运行的特殊容器，主要用于用户发起的故障排查操作（如调试），不适用于构建应用程序。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/ephemeral-containers.md`）*

---

### Horizontal Pod Autoscaling

HorizontalPodAutoscaler（HPA）是 Kubernetes 的 API 资源和控制器，可根据观察到的指标（如 CPU 利用率、内存利用率或自定义指标）自动调整工作负载（Deployment、StatefulSet 等）的副本数量。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/horizontal-pod-autoscaling.md`）*

---

### 容器镜像（Images）

容器镜像（Container Image）是封装了应用程序及其所有软件依赖项的二进制数据，是一个可独立运行的可执行软件包，并对其运行时环境做出非常明确的假设。在 Kubernetes 中，通常需要先创建应用程序的容器镜像并推送到镜像仓库，然后在 Pod 中引用该镜像。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/images.md`）*

---

### Init Containers

Init 容器是在 Pod 启动期间、于应用容器之前运行的特殊容器。它们通常用于执行应用镜像中不包含的初始化工具或设置脚本。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/init-containers.md`）*

---

### Jobs

Job 用于表示一次性任务，运行到完成即停止。Job 会创建一个或多个 Pod，并在达到指定成功完成数后结束。若 Pod 失败，Job 会根据配置进行重试。删除 Job 会级联删除其创建的 Pod。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/jobs.md`）*

---

### Managing Workloads

本页介绍在 Kubernetes 中部署应用后，如何使用各种工具和实践来管理、更新和扩展工作负载，涵盖 kubectl 批量操作、应用更新、金丝雀发布、资源注解和扩缩容等内容。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/managing-workloads.md`）*

---

### Pod Group Policies

Pod Group Policies 是 Workload API 的组成部分（Alpha，v1.35 默认禁用）。Workload 中定义的每个 Pod 组都必须声明一个调度策略，该策略决定调度器如何处理该组 Pod 的集合。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/pod-group-policies.md`）*

---

### Pod Hostname

本页解释 Pod 主机名的设置方式、配置后的潜在副作用以及底层机制。Pod 内部观察到的主机名默认来自 `metadata.name`。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/pod-hostname.md`）*

---

### Pod Lifecycle

Pod 遵循一个确定的生命周期，从 `Pending` 阶段开始，如果至少一个主容器正常启动则进入 `Running`，最终根据容器终止情况进入 `Succeeded` 或 `Failed` 阶段。Pod 被视为相对短暂（ephemeral）的实体。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/pod-lifecycle.md`）*

---

### Pod Quality of Service Classes

Kubernetes 根据 Pod 内容器的资源请求（requests）和限制（limits）为每个 Pod 分配一个服务质量（QoS）等级。该等级用于在节点资源不足时决定驱逐优先级。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/pod-quality-of-service-classes.md`）*

---

### Pods

Pod 是 Kubernetes 中最小的可部署计算单元，它是一组共享存储和网络资源、并协同运行的一个或多个容器的集合。Pod 中的容器始终被共位（co-located）和共调度（co-scheduled），在共享上下文中运行，相当于一个应用专属的"逻辑主机"。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/pods.md`）*

---

### ReplicaSet

ReplicaSet 的作用是维护一组稳定运行的 Pod 副本。它通常不直接使用，而是由 Deployment 自动管理，作为 Deployment 实现 Pod 创建、更新和扩缩容的底层机制。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/replicaset.md`）*

---

### ReplicationController

ReplicationController 是一种遗留 API，用于确保指定数量的 Pod 副本始终处于运行状态。它已被 Deployment 和 ReplicaSet 取代，仅在维护旧系统或学习 Kubernetes 历史时可能遇到。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/replicationcontroller.md`）*

---

### 运行时类（RuntimeClass）

RuntimeClass 是 Kubernetes 中用于选择容器运行时配置的特性（自 v1.20 起进入 Stable）。它允许用户为不同的 Pod 指定不同的容器运行时配置，从而在性能与安全性之间取得平衡。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/runtime-class.md`）*

---

### Sidecar Containers

Sidecar 容器是与主应用容器运行在同一 Pod 内的辅助容器，用于增强或扩展主应用功能，如日志收集、监控、安全代理或数据同步。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/sidecar-containers.md`）*

---

### Spot 与可抢占工作负载

在云原生环境中，**Spot 实例（AWS）、Preemptible VM（GCP）和 Low-priority VM（Azure）** 是云厂商以大幅折扣出售的闲置计算容量。2026 年的最佳实践表明，通过将**容错型工作负载**（如 AI 训练、批处理、CI/CD）部署到 Spot 实例上，企业可将计算成本降低 **50%–90%**。Kubernetes 结合 Kueue、Cluster Autoscaler 和 checkpoint 机制，已能安全、自动化地管理可抢占工作负载的生命周期。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/spot-and-preemptible-workloads.md`）*

---

### StatefulSets

StatefulSet 是用于管理有状态应用的工作负载 API 对象。它管理一组基于相同容器规范运行的 Pod，并保证这些 Pod 的排序和唯一性。与 Deployment 不同，StatefulSet 为每个 Pod 维护一个粘性标识（sticky identity），即使 Pod 被重新调度，该标识也不会改变。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/statefulsets.md`）*

---

### User Namespaces

用户命名空间（User Namespaces）是 Linux 的一项特性，用于将容器内的用户与主机（节点）上的用户隔离。容器内以 root 运行的进程，在主机上可映射为非 root 用户，从而显著降低容器逃逸后对主机或其他 Pod 的危害。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/user-namespaces.md`）*

---

### Vertical Pod Autoscaling

VerticalPodAutoscaler（VPA）自动调整工作负载（如 Deployment、StatefulSet）中 Pod 的资源请求（requests）和限制（limits），以匹配实际资源使用情况。这种垂直缩放也称为 rightsizing 或 autopilot。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/vertical-pod-autoscaling.md`）*

---

### Workload API

Workload API 是 Kubernetes v1.35 引入的 Alpha 特性（默认禁用，需启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组）。它提供了一种结构化的、机器可读的多 Pod 应用调度需求定义，补充了现有工作负载控制器的运行时行为。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/workload-api.md`）*

---

### Workload Management

Kubernetes 提供多个内置 API 用于声明式地管理工作负载及其组件。虽然应用最终运行在 Pod 中，但直接管理单个 Pod 非常繁琐。工作负载对象提供了更高层次的抽象，控制平面会根据定义自动管理 Pod 的生命周期。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/workload-management.md`）*

---

### Workload Reference

Workload Reference 是 Kubernetes v1.35 引入的 Alpha 特性（默认禁用，需启用 `GenericWorkload` 特性门控）。它允许将 Pod 链接到一个 Workload 对象，使调度器能够按组进行协同调度决策，而不是将 Pod 视为独立个体。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/workloads/workload-reference.md`）*

---

## 相关页面

- [[实体/k8s-workloads-domain-guide.md|k8s-workloads-domain-guide]]
- [[实体/k8s-workload-management.md|k8s-workload-management]]

## 来源文件

- `系统基础/topic-dictionary/workloads/advanced-pod-configuration.md`
- `系统基础/topic-dictionary/workloads/automatic-cleanup-for-finished-jobs.md`
- `系统基础/topic-dictionary/workloads/autoscaling-workloads.md`
- `系统基础/topic-dictionary/workloads/container-environment.md`
- `系统基础/topic-dictionary/workloads/container-lifecycle-hooks.md`
- `系统基础/topic-dictionary/workloads/container-runtime-interface-cri.md`
- `系统基础/topic-dictionary/workloads/cronjob.md`
- `系统基础/topic-dictionary/workloads/daemonset.md`
- `系统基础/topic-dictionary/workloads/deployments.md`
- `系统基础/topic-dictionary/workloads/disruptions.md`
- `系统基础/topic-dictionary/workloads/downward-api.md`
- `系统基础/topic-dictionary/workloads/ephemeral-containers.md`
- `系统基础/topic-dictionary/workloads/horizontal-pod-autoscaling.md`
- `系统基础/topic-dictionary/workloads/images.md`
- `系统基础/topic-dictionary/workloads/init-containers.md`
- `系统基础/topic-dictionary/workloads/jobs.md`
- `系统基础/topic-dictionary/workloads/managing-workloads.md`
- `系统基础/topic-dictionary/workloads/pod-group-policies.md`
- `系统基础/topic-dictionary/workloads/pod-hostname.md`
- `系统基础/topic-dictionary/workloads/pod-lifecycle.md`
- `系统基础/topic-dictionary/workloads/pod-quality-of-service-classes.md`
- `系统基础/topic-dictionary/workloads/pods.md`
- `系统基础/topic-dictionary/workloads/replicaset.md`
- `系统基础/topic-dictionary/workloads/replicationcontroller.md`
- `系统基础/topic-dictionary/workloads/runtime-class.md`
- `系统基础/topic-dictionary/workloads/sidecar-containers.md`
- `系统基础/topic-dictionary/workloads/spot-and-preemptible-workloads.md`
- `系统基础/topic-dictionary/workloads/statefulsets.md`
- `系统基础/topic-dictionary/workloads/user-namespaces.md`
- `系统基础/topic-dictionary/workloads/vertical-pod-autoscaling.md`
- `系统基础/topic-dictionary/workloads/workload-api.md`
- `系统基础/topic-dictionary/workloads/workload-management.md`
- `系统基础/topic-dictionary/workloads/workload-reference.md`

## Related

- [[deployment]] — Deployment
- [[实体/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[pod-lifecycle]] — Pod Lifecycle


<!-- risk-assessed -->
