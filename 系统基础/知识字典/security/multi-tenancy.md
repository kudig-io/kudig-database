---
title: 多租户
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- coredns
- rbac
- networkpolicy
- crd
- operator
- webhook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 多租户 是什么
- 如何 多租户
trigger_keywords:
- 多租户
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 多租户

## 概述

共享 [[Kubernetes|Kubernetes]] 集群可以节省成本并简化管理，但也带来了安全、公平性和“吵闹邻居”（noisy neighbors）等方面的挑战。集群共享可以有多种形式：不同应用运行在同一集群中，或同一应用的不同实例（面向不同终端用户）运行在同一集群中。这些共享方式通常统称为**多租户（multi-tenancy）**。虽然 Kubernetes 没有原生的“租户”或“终端用户”一等概念，但它提供了多种功能来帮助管理不同的租户需求。

## 核心概念/原理

Kubernetes 中的多租户通常分为两大类：

- **多团队（Multiple teams）**：组织内的多个团队共享一个集群，每个团队可能运行一个或多个工作负载。成员通常直接或通过 GitOps 控制器访问 Kubernetes 资源。团队之间通常存在一定信任，但需要 RBAC、配额和网络策略来安全、公平地共享集群。
- **多客户（Multiple customers / SaaS）**：SaaS 供应商为不同客户运行多个工作负载实例。客户通常不直接访问集群，Kubernetes 对其不可见。成本优化是关键，需要使用策略确保工作负载之间强隔离。

### 隔离概念

隔离水平通常用“硬多租户”（hard multi-tenancy，强隔离）和“软多租户”（soft multi-tenancy，弱隔离）来描述。实际上，隔离更像是一个连续光谱，包含多种技术：

- **控制平面隔离**：确保不同租户无法访问或影响彼此的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 资源。
- **数据平面隔离**：确保不同租户的 Pod 和工作负载之间有足够的隔离。

## 关键机制或特性

### 控制平面隔离

- **命名空间（Namespaces）**：
  - 提供 API 资源的分组隔离，允许不同租户使用相同名称的资源。
  - 许多安全策略（RBAC、[[NetworkPolicy|NetworkPolicy]]）以命名空间为范围。
  - 最佳实践是为每个工作负载分配独立的命名空间。
- **访问控制（RBAC）**：
  - 使用 Role 和 RoleBinding 在命名空间级别限制租户访问。
  - 遵循最小权限原则，防止租户访问其他命名空间的资源。
- **资源配额（Resource Quotas）**：
  - 限制租户可使用的 CPU、内存以及 API 对象数量。
  - 防止单个租户垄断资源或压垮控制平面（吵闹邻居问题）。

### 数据平面隔离

- **网络隔离（Network Policies）**：
  - 默认情况下集群内所有 Pod 可互相通信。NetworkPolicy 可限制 Pod 间通信。
  - 建议采用“默认拒绝所有”策略，然后按需添加允许规则。
  - 服务网格（Service Mesh）可提供更细粒度的 L7 策略和 mTLS 加密。
- **存储隔离（Storage Isolation）**：
  - 推荐使用动态卷配置，避免使用依赖节点资源的卷类型。
  - StorageClass 和 PersistentVolumeClaim 可用于为不同租户隔离存储。
  - 若共享 StorageClass，建议设置回收策略为 `Delete`，防止 PV 跨命名空间重用。
- **沙箱容器（Sandboxing）**：
  - 容器提供 OS 级虚拟化，隔离边界弱于虚拟机。
  - 沙箱容器（如 gVisor、Kata Containers）通过在独立的执行环境（虚拟机或用户空间内核）中运行 Pod 来增强隔离，适合运行不受信任的代码。
- **节点隔离（Node Isolation）**：
  - 将特定节点专门分配给某个租户，禁止不同租户的 Pod 混合运行。
  - 可使用 Taints/Tolerations 和 mutating webhook 自动将租户 Pod 调度到专属节点上。
  - 降低了容器逃逸后的横向移动风险，也便于按节点计费。

### 其他考虑因素

- **API 优先级和公平性（API Priority and Fairness）**：为不同 Pod 分配 API 调用优先级，防止低优先级应用挤占控制平面资源。
- **服务质量（QoS）**：通过网络 QoS、存储类、Pod 优先级和抢占机制，为不同租户提供差异化的服务等级。
- **DNS**：默认情况下 Kubernetes DNS 允许跨命名空间查询。在多租户环境中，可能需要通过 CoreDNS 配置限制跨命名空间 DNS 查找。
- **Operators**：多租户环境中使用的 Operator 应支持在租户命名空间中创建资源、配置资源限制以及支持数据平面隔离技术。

### 实现方式

- **每个租户一个命名空间**：资源开销可忽略，支持租户间的服务通信，但无法隔离非命名空间资源（如 CRD、StorageClass、Webhook）。
- **每个租户一个虚拟控制平面**：通过 Kubernetes 扩展为每个租户提供独立的 API server、控制器管理器和 etcd。解决了命名空间隔离的局限，但资源开销更大，跨租户共享更复杂。数据平面隔离仍需单独实施。

## 使用场景

- 多个开发/运维团队共享一个内部 Kubernetes 集群。
- SaaS 提供商在一个集群中为大量客户运行隔离的应用实例。
- 需要在成本和隔离强度之间进行权衡的混合场景。

## 最佳实践/注意事项

- 即使在使用虚拟控制平面或专用集群时，也建议将每个工作负载放在独立的命名空间中，以便应用细粒度的安全策略。
- 默认拒绝所有入站和出站流量，然后根据业务需求显式放行。
- 对于不信任的代码或高敏感性工作负载，优先考虑沙箱容器或节点隔离。
- 定期审查 RBAC、ResourceQuota 和 NetworkPolicy 配置，确保其符合当前租户需求。
- 在评估隔离方案时，权衡安全性、成本、运维复杂性和性能开销。

## 参考链接

- https://kubernetes.io/docs/concepts/security/multi-tenancy/

## Related
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
