---
title: K8s 配置管理术语参考
description: '# K8s 配置管理术语参考'
summary: '本页汇总了 **配置管理** 领域的 6 个 Kubernetes 术语定义与概念说明。'
category: references
tags:
- k8s
- dictionary
- configuration
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 配置管理术语参考 是什么
- 如何 K8s 配置管理术语参考
trigger_keywords:
- K8s
- 配置管理术语参考
prerequisites:
- kubectl-basics
---



# K8s 配置管理术语参考

本页汇总了 **配置管理** 领域的 6 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[entities/k8s-architecture-fundamentals.md|k8s-architecture-fundamentals]] | [[entities/k8s-control-plane-deep-dive.md|k8s-control-plane-deep-dive]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **ConfigMaps** | Configmaps | ConfigMap 是 Kubernetes 中用于存储非机密数据的 API 对象，以键值对（key-value）形式保存 |
| **Liveness, Readiness, and Startup Probes** | Liveness Readiness And Startup Probes | Kubernetes 提供三种探针（Probe）来持续监控 Pod 中容器的健康状态 |
| **Organizing Cluster Access Using kubeconfig Files** | Organizing Cluster Access Using Kubeconfig Files | kubeconfig 文件用于组织关于集群、用户、命名空间和认证机制的信息 |
| **Resource Management for Pods and Containers** | Resource Management For Pods And Containers | 在 Kubernetes 中，你可以为 Pod 中的每个容器指定所需的资源量 |
| **Resource Management for Windows nodes** | Resource Management For Windows Nodes | 本文档概述了 Linux 与 Windows 节点在资源管理方面的差异 |
| **Secrets** | Secrets | Secret 是 Kubernetes 中用于存储敏感数据（如密码、令牌、密钥等）的 API 对象 |

---

### ConfigMaps

ConfigMap 是 Kubernetes 中用于存储非机密数据的 API 对象，以键值对（key-value）形式保存。Pod 可以将 ConfigMap 用作环境变量、命令行参数，或者作为卷中的配置文件。通过 ConfigMap，你可以将环境相关的配置与容器镜像解耦，使应用更易于移植。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/configuration/configmaps.md`）*

---

### Liveness, Readiness, and Startup Probes

Kubernetes 提供三种探针（Probe）来持续监控 Pod 中容器的健康状态。根据探针返回的结果，Kubernetes 可以决定是否需要重启不健康的容器，或者是否将流量路由到尚未就绪的容器。这三种探针分别是：Startup Probe（启动探针）、Liveness Probe（存活探针）和 Readiness Probe（就绪探针）。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/configuration/liveness-readiness-and-startup-probes.md`）*

---

### Organizing Cluster Access Using kubeconfig Files

kubeconfig 文件用于组织关于集群、用户、命名空间和认证机制的信息。`kubectl` 命令行工具通过读取 kubeconfig 文件来获取连接集群所需的参数，从而选择合适的集群并与 API Server 通信。需要注意的是，"kubeconfig" 是一种通用称谓，并不代表存在一个名为 `kubeconfig` 的特定文件。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/configuration/organizing-cluster-access-using-kubeconfig-files.md`）*

---

### Resource Management for Pods and Containers

在 Kubernetes 中，你可以为 Pod 中的每个容器指定所需的资源量。最常见的资源类型是 CPU 和内存（RAM）。通过设置 `requests`（请求）和 `limits`（限制），调度器可以为 Pod 选择合适的节点，而 kubelet 则确保运行中的容器不会超出设定的资源上限。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/configuration/resource-management-for-pods-and-containers.md`）*

---

### Resource Management for Windows nodes

本文档概述了 Linux 与 Windows 节点在资源管理方面的差异。由于操作系统内核和进程隔离机制的不同，Kubernetes 在 Windows 节点上的资源管理方式与 Linux 存在显著区别。了解这些差异对于在混合操作系统集群中正确配置和调度工作负载至关重要。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/configuration/resource-management-for-windows-nodes.md`）*

---

### Secrets

Secret 是 Kubernetes 中用于存储敏感数据（如密码、令牌、密钥等）的 API 对象。使用 Secret 可以避免将机密信息硬编码到 Pod 规约或容器镜像中，从而降低在创建、查看和编辑 Pod 过程中泄露敏感数据的风险。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/configuration/secrets.md`）*

---

## 相关页面

- [[entities/k8s-architecture-fundamentals.md|k8s-architecture-fundamentals]]
- [[entities/k8s-control-plane-deep-dive.md|k8s-control-plane-deep-dive]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/configuration/configmaps.md`
- `domain-17-system-foundation/topic-dictionary/configuration/liveness-readiness-and-startup-probes.md`
- `domain-17-system-foundation/topic-dictionary/configuration/organizing-cluster-access-using-kubeconfig-files.md`
- `domain-17-system-foundation/topic-dictionary/configuration/resource-management-for-pods-and-containers.md`
- `domain-17-system-foundation/topic-dictionary/configuration/resource-management-for-windows-nodes.md`
- `domain-17-system-foundation/topic-dictionary/configuration/secrets.md`

## Related

- [[entities/platform-engineering-terms.md|platform-engineering-terms]] — K8s 平台工程术语参考
- [[entities/tooling-terms.md|tooling-terms]] — K8s 工具链术语参考
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
