---
title: 服务质量
description: QoS（Quality of Service）是 Kubernetes 对 Pod 的优先级分类机制。当节点资源不足时，kubelet 根据
  QoS 类别决定驱...
summary: QoS（Quality of Service）是 Kubernetes 对 Pod 的优先级分类机制。当节点资源不足时，kubelet 根据 QoS
  类别决定驱...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- qos
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务质量 是什么
- QoS (Quality of Service) 详解
trigger_keywords:
- 服务质量
- QoS (Quality of Service)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务质量

> **英文名**: QoS (Quality of Service)

## 概述

QoS（Quality of Service）是 Kubernetes 对 Pod 的优先级分类机制。当节点资源不足时，kubelet 根据 QoS 类别决定驱逐 Pod 的顺序，优先级低的 Pod 先被驱逐。

## 核心概念/原理

### QoS 类别

| 类别 | 条件 | 驱逐优先级 |
|------|------|-----------|
| **Guaranteed** | 所有容器都设置了 requests == limits | 最低（最后被驱逐） |
| **Burstable** | 至少一个容器设置了 requests 或 limits | 中等 |
| **BestEffort** | 所有容器都没有设置 requests 和 limits | 最高（最先被驱逐） |

### 判定规则

- **Guaranteed**：每个容器的 cpu/memory 的 requests 和 limits 都相等（或只设置 limits 未设置 requests，此时 requests 默认等于 limits）。
- **BestEffort**：所有容器的 cpu/memory 都未设置 requests 和 limits。
- **Burstable**：不满足以上两种条件的 Pod。

## 关键机制或特性

- QoS 类别在 Pod 创建时确定，运行期间不可更改。
- 可以通过 `kubectl get pod -o jsonpath='{.status.qosClass}'` 查看。
- 节点压力驱逐时，优先驱逐 BestEffort → Burstable → Guaranteed。

## 使用场景与最佳实践

- 关键生产工作负载使用 Guaranteed QoS。
- 批处理和低优先级任务可以使用 Burstable。
- 避免在生产环境中使用 BestEffort（随时可能被驱逐）。
- 使用 `priorityClassName` 进一步细化驱逐优先级。

## 参考链接

- [QoS (Quality of Service) - Official Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/quality-service-pod/)

## Related

- [[系统基础/知识字典/scheduling/affinity.md|Affinity]]
- [[系统基础/知识字典/scheduling/anti-affinity.md|Anti Affinity]]
- [[系统基础/知识字典/scheduling/taint.md|Taint]]
- [[系统基础/知识字典/scheduling/toleration.md|Toleration]]
- [[系统基础/知识字典/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
