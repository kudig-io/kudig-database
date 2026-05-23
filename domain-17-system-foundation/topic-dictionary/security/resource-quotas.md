---
title: Resource Quotas（资源配额）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- job
- cronjob
- rbac
- gpu
- nvidia
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Resource Quotas（资源配额） 是什么
- 如何 Resource Quotas（资源配额）
trigger_keywords:
- Resource
- Quotas
- 资源配额
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Resource Quotas（资源配额）

## 概述

ResourceQuota 是 [[Kubernetes|Kubernetes]] 为管理员提供的一种工具，用于限制命名空间级别的聚合资源消耗。当多个用户或团队共享一个节点数量固定的集群时，ResourceQuota 可以防止某个团队使用超过其公平份额的资源。它不仅能限制计算资源、存储资源和扩展资源的总量，还能限制命名空间中各类 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] 对象的数量。

## 核心概念/原理

- **命名空间隔离**：不同团队在不同的命名空间中工作，这种隔离可通过 RBAC 等授权机制强制执行。
- **ResourceQuota 对象**：集群管理员为每个命名空间创建至少一个 ResourceQuota，配额系统会跟踪资源使用情况，确保不超过硬限制（hard limits）。
- **独立 admission plugin**：ResourceQuota 通过 API 服务器的 `--enable-admission-plugins` 中的 `ResourceQuota` 插件启用，大多数发行版默认已开启。
- **违反约束**：若创建或更新资源违反配额约束，控制平面将拒绝请求并返回 HTTP `403 Forbidden`。

## 关键机制或特性

### 1. 可限制的资源类型

- **基础设施资源**：`limits.cpu`、`limits.memory`、`requests.cpu`、`requests.memory`、`hugepages-<size>`、`cpu`、`memory`。
- **扩展资源**：仅允许带 `requests.` 前缀的配额项（如 `requests.nvidia.com/gpu: 4`），因为扩展资源不允许超售。
- **DRA 资源声明**：支持按设备类别限制（如 `examplegpu.deviceclass.resource.k8s.io/devices: 4`）。
- **存储资源**：`requests.storage`、`persistentvolumeclaims`、按 StorageClass 细分的存储请求和 PVC 数量。
- **临时存储**：`requests.ephemeral-storage`、`limits.ephemeral-storage`、`ephemeral-storage`（v1.8 Alpha 起）。
- **对象计数**：可限制特定 API 资源的对象总数，如 `count/pods`、`count/secrets`、`count/services`、`count/deployments.apps` 等；也支持专用语法如 `pods`、`configmaps`、`services.loadbalancers`、`services.nodeports`。

### 2. Quota Scopes（配额作用域）

通过 `scopes` 或 `scopeSelector` 可将配额仅应用于匹配特定条件的资源：

| Scope | 说明 |
|-------|------|
| `BestEffort` | 仅匹配 BestEffort QoS 的 Pod |
| `NotBestEffort` | 匹配 Guaranteed 或 Burstable QoS 的 Pod |
| `Terminating` | 匹配设置了 `.spec.activeDeadlineSeconds` 的 Pod |
| `NotTerminating` | 匹配未设置 `.spec.activeDeadlineSeconds` 的 Pod |
| `CrossNamespacePodAffinity` | 匹配设置了跨命名空间 Pod（反）亲和性的 Pod |
| `PriorityClass` | 匹配引用了指定优先级类的 Pod |
| `VolumeAttributesClass` | 匹配引用了指定卷属性类的 PVC |

`scopeSelector` 支持 `In`、`NotIn`、`Exists`、`DoesNotExist` 操作符。

### 3. 与集群容量的关系

ResourceQuota 以绝对单位表示，与集群容量无关。增加节点不会自动让每个命名空间消耗更多资源。如需动态调整配额，可编写控制器监听配额使用情况并按信号调整。

### 4. 对象创建与 Pod 创建的区别

创建 Deployment 等控制器对象时，即使其管理的 Pod 会超出配额，控制器的创建本身**可能成功**，但 Pod 可能无法实际创建。应通过 `kubectl describe` 查看控制器状态排查问题。

## 使用场景

- **多租户资源分配**：例如，在 32 GiB RAM / 16 核的集群中，为 Team A 分配 20 GiB / 10 核，为 Team B 分配 10 GiB / 4 核，并保留 2 GiB / 2 核。
- **环境隔离**：限制 "testing" 命名空间只能使用 1 核和 1 GiB RAM，而 "production" 命名空间不受限制。
- **保护控制平面存储**：通过限制 Secrets、ConfigMaps、Jobs 等对象数量，防止存储耗尽导致服务器和控制器无法启动。
- **限制跨命名空间亲和性**：使用 `CrossNamespacePodAffinity` scope 控制哪些命名空间允许配置跨命名空间的 Pod 亲和性/反亲和性。
- **按优先级类隔离**：为不同的 PriorityClass（如 low、medium、high）分别设置独立的资源配额，确保高优先级任务有充足的资源预算。

## 最佳实践/注意事项

- **与 LimitRange 配合使用**：ResourceQuota 要求 Pod 必须显式设置 CPU/内存的 request 或 limit；建议同时配置 LimitRange 为这些 Pod 自动注入默认值，避免用户遗漏。
- **临时存储配额的副作用**：使用 CRI 容器运行时，容器日志会计入临时存储配额，可能导致 Pod 因存储配额耗尽而被意外驱逐。
- **配额竞争**：当集群总容量小于所有命名空间配额之和时，资源竞争按先到先得（first-come-first-served）处理。
- **对象计数配额**：建议为 Secrets、Jobs（防止 CronJob 配置错误导致 DoS）等大型或高频对象设置数量限制。
- **防止配额被绕过**：集群管理员应限制普通用户删除或更新 ResourceQuota 对象的权限，例如通过 ValidatingAdmissionPolicy。

## 参考链接

- [Kubernetes 官方文档 - Resource Quotas](https://kubernetes.io/docs/concepts/policy/resource-quotas/)

## Related

- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]
