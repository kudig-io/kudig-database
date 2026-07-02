---
title: 有状态副本集
description: StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet
  为每个 Pod 提供...
summary: StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet 为每个
  Pod 提供...
category: dictionary
tags:
- k8s
- glossary
- statefulset
- workload
- storage
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 有状态副本集 是什么
- StatefulSet 详解
trigger_keywords:
- 有状态副本集
- StatefulSet
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 有状态副本集

> **英文名**: StatefulSet

## 概述

StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet 为每个 Pod 提供稳定的网络标识、存储和有序的部署/扩缩容/删除顺序。

## 核心概念/原理

### 核心特性

- **稳定的网络标识**：每个 Pod 有固定的名称（如 `mysql-0`, `mysql-1`）和对应的 Headless Service DNS。
- **稳定的存储**：每个 Pod 通过 VolumeClaimTemplate 绑定独立的 PVC，Pod 重启/重调度后仍保持绑定。
- **有序操作**：Pod 按序号顺序创建（0→N-1），逆序删除（N-1→0）。
- **有序更新**：RollingUpdate 从高序号向低序号逆序更新。

### 与 Deployment 的对比

| 特性 | Deployment | StatefulSet |
|------|-----------|-------------|
| Pod 标识 | 随机名称 | 固定有序名称 |
| 存储 | 共享或无 | 每 Pod 独立 PVC |
| 创建顺序 | 并行 | 有序（0→N-1） |
| 适用场景 | 无状态应用 | 有状态应用 |

## 关键机制或特性

- `podManagementPolicy: Parallel` 可让 Pod 并行创建/删除。
- `serviceName` 必须指向一个 Headless Service。
- 删除 StatefulSet 不会自动删除关联的 PVC（保护数据安全）。

## 使用场景与最佳实践

- 数据库（MySQL、PostgreSQL）、消息队列（Kafka）、分布式存储等使用 StatefulSet。
- 为每个 Pod 配置独立的 PVC 和 VolumeClaimTemplate。
- 使用 `partition` 字段实现金丝雀更新。
- 考虑使用 Operator 模式管理复杂的有状态应用生命周期。

## 参考链接

- [StatefulSet - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/)

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/workloads/daemonset.md|Daemonset]]
- [[domain-17-system-foundation/topic-dictionary/workloads/replicaset.md|Replicaset]]
- [[domain-17-system-foundation/topic-dictionary/workloads/job.md|Job]]


<!-- risk-assessed -->
