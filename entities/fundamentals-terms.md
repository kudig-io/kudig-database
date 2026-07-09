---
title: K8s 基础概念术语参考
description: '| **Cloud Controller Manager（云控制器管理器）** | Cloud Controller Manager |
  Cloud Controller Manager 是 Kubernetes 控制平面的一个组件，它将云厂商特定的控制逻辑嵌入到 Kubernetes 中 |'
summary: '| **Cloud Controller Manager（云控制器管理器）** | Cloud Controller Manager | Cloud
  Controller Manager 是 Kubernetes 控制平面的一个组件，它将云厂商特定的控制逻辑嵌入到 Kubernetes 中 |'
category: references
tags:
- k8s
- dictionary
- fundamentals
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 基础概念术语参考 是什么
- 如何 K8s 基础概念术语参考
trigger_keywords:
- K8s
- 基础概念术语参考
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 基础概念术语参考

本页汇总了 **基础概念** 领域的 24 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[entities/k8s-architecture-fundamentals.md|k8s-architecture-fundamentals]] | [[entities/k8s-knowledge-map.md|k8s-knowledge-map]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **About cgroup v2（关于 cgroup v2）** | About Cgroup V2 | 在 Linux 上，控制组（control groups，简称 cgroups）用于限制分配给进程的资源 |
| **注解** | Annotations | Kubernetes 注解（Annotations）用于将任意非标识性元数据附加到对象上 |
| **Cloud Controller Manager（云控制器管理器）** | Cloud Controller Manager | Cloud Controller Manager 是 Kubernetes 控制平面的一个组件，它将云厂商特定的控制逻辑嵌入到 Kubernetes 中 |
| **Communication between Nodes and the Control Plane（节点与控制平面之间的通信）** | Communication Between Nodes And The Control Plane | 本文档梳理了 Kubernetes 集群中 API 服务器与节点之间的所有通信路径，目的是帮助用户根据安全需求自定义网络配置，使集群能够在不受信任的网络（... |
| **Controllers（控制器）** | Controllers | 在 Kubernetes 中，控制器是监控集群状态的控制循环（control loop） |
| **字段选择器** | Field Selectors | 字段选择器（Field Selectors）允许根据一个或多个资源字段的值来选择 Kubernetes 对象 |
| **Finalizers** | Finalizers | Finalizers 是带有命名空间限制的键，用于告诉 Kubernetes 在完全删除标记为删除的资源之前等待特定条件满足 |
| **Garbage Collection（垃圾回收）** | Garbage Collection | 垃圾回收（Garbage Collection）是 Kubernetes 用于清理集群资源的各种机制的统称 |
| **Kubernetes 组件** | Kubernetes Components | Kubernetes 集群由控制平面（Control Plane）和一组工作节点（Worker Nodes）组成 |
| **** | Kubernetes Concepts Reference | title: Kubernetes Concepts Reference
description: '| **适合读者** | 初学者（查概念）→ 中级（... |
| **Kubernetes 对象管理** | Kubernetes Object Management | `kubectl` 命令行工具支持多种方式来创建和管理 Kubernetes 对象 |
| **Kubernetes Self-Healing（Kubernetes 自愈能力）** | Kubernetes Self Healing | Kubernetes 从设计之初就具备自愈能力，以帮助维护工作负载的健康和可用性 |
| **标签和选择器** | Labels And Selectors | 标签（Labels）是附加到对象（如 Pod）上的键/值对，用于指定对用户有意义的相关标识属性 |
| **Leases（租约）** | Leases | Lease（租约）是分布式系统中用于锁定共享资源和协调集合成员活动的机制 |
| **Mixed Version Proxy（混合版本代理）** | Mixed Version Proxy | Mixed Version Proxy 是 Kubernetes 1 |
| **命名空间** | Namespaces | 在 Kubernetes 中，命名空间（Namespaces）提供了一种在单个集群内隔离资源组的机制 |
| **Nodes（节点）** | Nodes | Node（节点）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机 |
| **对象名称和 ID** | Object Names And Ids | 集群中的每个对象都有一个在同类资源中唯一的名称（Name），以及一个在整个集群中唯一的 UID |
| **Kubernetes 中的对象** | Objects In Kubernetes | Kubernetes 对象是 Kubernetes 系统中持久存在的实体 |
| **所有者和依赖者** | Owners And Dependents | 在 Kubernetes 中，一些对象是所有者（owners），而另一些对象是它们的依赖者（dependents） |
| **推荐标签** | Recommended Labels | 除了 `kubectl` 和 Dashboard 之外，还有许多工具可以可视化和管理 Kubernetes 对象 |
| **存储版本** | Storage Versions | Kubernetes API 服务器将对象存储在 etcd（或兼容的键值存储）中 |
| **kubectl 命令行工具** | The Kubectl Command Line Tool | `kubectl` 是与 Kubernetes 集群的控制平面进行通信的主要命令行工具 |
| **Kubernetes API** | The Kubernetes Api | Kubernetes API 是查询和操作 Kubernetes 中对象状态的核心机制 |

---

### About cgroup v2（关于 cgroup v2）

在 Linux 上，控制组（control groups，简称 cgroups）用于限制分配给进程的资源。kubelet 和底层容器运行时需要通过 cgroups 来强制执行 Pod 和容器的资源管理，包括 CPU/内存的请求（requests）和限制（limits）。Linux 上有两个版本的 cgroups：cgroup v1 和 cgroup v2。cgroup v2 是新一代的 cgroup API。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/about-cgroup-v2.md`）*

---

### 注解

Kubernetes 注解（Annotations）用于将任意非标识性元数据附加到对象上。与标签不同，注解不用于识别和选择对象，但可以包含标签不允许的字符，大小和结构也更灵活。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/annotations.md`）*

---

### Cloud Controller Manager（云控制器管理器）

Cloud Controller Manager 是 Kubernetes 控制平面的一个组件，它将云厂商特定的控制逻辑嵌入到 Kubernetes 中。它使集群能够连接到云提供商的 API，并将与云平台交互的组件与仅与集群交互的组件分离开来，从而实现解耦，允许云厂商以不同于主 Kubernetes 项目的节奏发布新特性。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/cloud-controller-manager.md`）*

---

### Communication between Nodes and the Control Plane（节点与控制平面之间的通信）

本文档梳理了 Kubernetes 集群中 API 服务器与节点之间的所有通信路径，目的是帮助用户根据安全需求自定义网络配置，使集群能够在不受信任的网络（或公有云的公网 IP）上运行。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/communication-between-nodes-and-the-control-plane.md`）*

---

### Controllers（控制器）

在 Kubernetes 中，控制器是监控集群状态的控制循环（control loop）。它们持续比较集群的**当前状态（current state）**与**期望状态（desired state）**，并在必要时采取措施使当前状态向期望状态靠拢。控制器本身通常不直接执行操作，而是通过向 API 服务器发送请求来产生副作用。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/controllers.md`）*

---

### 字段选择器

字段选择器（Field Selectors）允许根据一个或多个资源字段的值来选择 Kubernetes 对象。与标签选择器不同，字段选择器基于资源的实际字段值进行过滤，是一种更底层的资源筛选机制。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/field-selectors.md`）*

---

### Finalizers

Finalizers 是带有命名空间限制的键，用于告诉 Kubernetes 在完全删除标记为删除的资源之前等待特定条件满足。Finalizers 会通知控制器清理被删除对象所拥有的资源。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/finalizers.md`）*

---

### Garbage Collection（垃圾回收）

垃圾回收（Garbage Collection）是 Kubernetes 用于清理集群资源的各种机制的统称。它允许自动清理以下类型的资源：已终止的 Pod、已完成的 Job、没有 owner reference 的对象、未使用的容器和镜像、回收策略为 Delete 的动态供给 PersistentVolume、过期或陈旧的 CertificateSigningRequest（CSR）、以及已被删除的节点和节点 Lease 对象等。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/garbage-collection.md`）*

---

### Kubernetes 组件

Kubernetes 集群由控制平面（Control Plane）和一组工作节点（Worker Nodes）组成。每个组件各司其职，协同工作以维护集群的期望状态。本文档对 Kubernetes 的核心组件进行高层概述。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/kubernetes-components.md`）*

---

### 

title: Kubernetes Concepts Reference
description: '| **适合读者** | 初学者（查概念）→ 中级（理解原理）→ 专家（深度参考） |'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 90min
intent_queries:
- Kubernetes Concepts Reference 是什么
- 如何 Kubernetes Concepts Reference
trigger_keywords:
- Kubernetes
- Concepts
- Reference
- dic...

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/kubernetes-concepts-reference.md`）*

---

### Kubernetes 对象管理

`kubectl` 命令行工具支持多种方式来创建和管理 Kubernetes 对象。本文档概述了不同的管理方法及其适用场景，帮助用户选择适合自身工作流的对象管理方式。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/kubernetes-object-management.md`）*

---

### Kubernetes Self-Healing（Kubernetes 自愈能力）

Kubernetes 从设计之初就具备自愈能力，以帮助维护工作负载的健康和可用性。当容器失败、节点不可用时，Kubernetes 能够自动进行恢复操作，并确保系统始终维持期望状态。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/kubernetes-self-healing.md`）*

---

### 标签和选择器

标签（Labels）是附加到对象（如 Pod）上的键/值对，用于指定对用户有意义的相关标识属性。与注解不同，标签可用于组织和选择对象的子集。标签选择器（Label Selectors）是 Kubernetes 中核心的分组原语。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/labels-and-selectors.md`）*

---

### Leases（租约）

Lease（租约）是分布式系统中用于锁定共享资源和协调集合成员活动的机制。在 Kubernetes 中，Lease 对象属于 `coordination.k8s.io` API 组，被用于系统级关键能力，如节点心跳（node heartbeats）和组件级领导者选举（leader election）。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/leases.md`）*

---

### Mixed Version Proxy（混合版本代理）

Mixed Version Proxy 是 Kubernetes 1.28 引入的 Alpha 特性（默认关闭），它允许 API 服务器将资源请求代理给其他对等（peer）API 服务器，同时使客户端能够通过发现机制获得整个集群资源的完整视图。这在集群中运行多个不同版本的 Kubernetes API 服务器时非常有用（例如在进行长时间的滚动升级期间）。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/mixed-version-proxy.md`）*

---

### 命名空间

在 Kubernetes 中，命名空间（Namespaces）提供了一种在单个集群内隔离资源组的机制。资源名称需要在命名空间内唯一，但不必跨命名空间唯一。命名空间作用域仅适用于命名空间资源（如 Deployment、Service），不适用于集群范围资源（如 StorageClass、Node、PersistentVolume）。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/namespaces.md`）*

---

### Nodes（节点）

Node（节点）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机。Kubernetes 通过将容器放入 Pod 中，并在 Node 上运行这些 Pod 来执行工作负载。一个集群通常包含多个节点，但在学习或资源受限的环境中也可能只有一个节点。每个节点由控制平面管理，包含运行 Pod 所必需的服务组件：kubelet、容器运行时（container runtime）和 kube-proxy。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/nodes.md`）*

---

### 对象名称和 ID

集群中的每个对象都有一个在同类资源中唯一的名称（Name），以及一个在整个集群中唯一的 UID。名称用于在资源 URL 中引用对象，而 UID 用于区分集群生命周期内所有对象的历史实例。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/object-names-and-ids.md`）*

---

### Kubernetes 中的对象

Kubernetes 对象是 Kubernetes 系统中持久存在的实体。Kubernetes 使用这些实体来表示集群的状态。通过创建、修改或删除对象，用户可以向 Kubernetes 系统传达期望的集群状态。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/objects-in-kubernetes.md`）*

---

### 所有者和依赖者

在 Kubernetes 中，一些对象是所有者（owners），而另一些对象是它们的依赖者（dependents）。例如，ReplicaSet 是一组 Pod 的所有者。所有权与标签和选择器机制不同，它帮助 Kubernetes 的不同部分避免干扰它们不控制的对象。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/owners-and-dependents.md`）*

---

### 推荐标签

除了 `kubectl` 和 Dashboard 之外，还有许多工具可以可视化和管理 Kubernetes 对象。一组通用的推荐标签（Recommended Labels）允许这些工具以可互操作的方式工作，用所有工具都能理解的通用方式描述对象。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/recommended-labels.md`）*

---

### 存储版本

Kubernetes API 服务器将对象存储在 etcd（或兼容的键值存储）中。每个对象使用特定版本的 API 类型进行序列化。Kubernetes 使用"存储版本"（storage version）这一术语来描述对象在集群中的实际存储方式。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/storage-versions.md`）*

---

### kubectl 命令行工具

`kubectl` 是与 Kubernetes 集群的控制平面进行通信的主要命令行工具。它通过 Kubernetes API 发送请求，是用户管理集群资源、检查集群状态和调试应用的主要接口。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/the-kubectl-command-line-tool.md`）*

---

### Kubernetes API

Kubernetes API 是查询和操作 Kubernetes 中对象状态的核心机制。Kubernetes 控制平面的核心是 API 服务器及其暴露的 HTTP API。用户、集群内部的不同部分以及外部组件都通过 API 服务器相互通信。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/fundamentals/the-kubernetes-api.md`）*

---

## 相关页面

- [[entities/k8s-architecture-fundamentals.md|k8s-architecture-fundamentals]]
- [[entities/k8s-knowledge-map.md|k8s-knowledge-map]]

## 来源文件

- `系统基础/topic-dictionary/fundamentals/about-cgroup-v2.md`
- `系统基础/topic-dictionary/fundamentals/annotations.md`
- `系统基础/topic-dictionary/fundamentals/cloud-controller-manager.md`
- `系统基础/topic-dictionary/fundamentals/communication-between-nodes-and-the-control-plane.md`
- `系统基础/topic-dictionary/fundamentals/controllers.md`
- `系统基础/topic-dictionary/fundamentals/field-selectors.md`
- `系统基础/topic-dictionary/fundamentals/finalizers.md`
- `系统基础/topic-dictionary/fundamentals/garbage-collection.md`
- `系统基础/topic-dictionary/fundamentals/kubernetes-components.md`
- `系统基础/topic-dictionary/fundamentals/kubernetes-concepts-reference.md`
- `系统基础/topic-dictionary/fundamentals/kubernetes-object-management.md`
- `系统基础/topic-dictionary/fundamentals/kubernetes-self-healing.md`
- `系统基础/topic-dictionary/fundamentals/labels-and-selectors.md`
- `系统基础/topic-dictionary/fundamentals/leases.md`
- `系统基础/topic-dictionary/fundamentals/mixed-version-proxy.md`
- `系统基础/topic-dictionary/fundamentals/namespaces.md`
- `系统基础/topic-dictionary/fundamentals/nodes.md`
- `系统基础/topic-dictionary/fundamentals/object-names-and-ids.md`
- `系统基础/topic-dictionary/fundamentals/objects-in-kubernetes.md`
- `系统基础/topic-dictionary/fundamentals/owners-and-dependents.md`
- `系统基础/topic-dictionary/fundamentals/recommended-labels.md`
- `系统基础/topic-dictionary/fundamentals/storage-versions.md`
- `系统基础/topic-dictionary/fundamentals/the-kubectl-command-line-tool.md`
- `系统基础/topic-dictionary/fundamentals/the-kubernetes-api.md`

## Related

- [[deployment]] — Deployment
- [[entities/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
