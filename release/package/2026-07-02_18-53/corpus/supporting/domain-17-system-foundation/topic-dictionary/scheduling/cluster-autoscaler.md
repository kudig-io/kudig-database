---
title: Cluster Autoscaler
description: Cluster Autoscaler（CA）是 Kubernetes 官方的节点级自动扩缩容组件。当 Pod 因资源不足无法调度时自动扩容节点，当节点资源长期空...
summary: Cluster Autoscaler（CA）是 Kubernetes 官方的节点级自动扩缩容组件。当 Pod 因资源不足无法调度时自动扩容节点，当节点资源长期空...
category: dictionary
tags:
- k8s
- glossary
- cluster-autoscaler
- autoscaling
- node
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cluster Autoscaler 是什么
- Cluster Autoscaler 详解
trigger_keywords:
- Cluster Autoscaler
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cluster Autoscaler

> **英文名**: Cluster Autoscaler

## 概述

Cluster Autoscaler（CA）是 Kubernetes 官方的节点级自动扩缩容组件。当 Pod 因资源不足无法调度时自动扩容节点，当节点资源长期空闲时自动缩容节点，优化集群成本。

## 核心概念/原理

### 与 HPA/VPA/KEDA 对比

| 工具 | 扩缩目标 | 触发条件 |
|------|----------|----------|
| HPA | Pod 副本数 | CPU/Memory/Custom Metrics |
| VPA | Pod 资源请求 | 历史资源使用 |
| KEDA | Pod 副本数 | 外部事件源 |
| **Cluster Autoscaler** | **节点数量** | **Pending Pods / 空闲节点** |

### 扩容流程

Pending Pod → CA 检测 → 请求云厂商创建节点 → 节点加入集群 → Pod 调度

## 关键机制或特性

- **Scale-Up**：检测 Pending Pod，模拟调度找到合适的节点组扩容。
- **Scale-Down**：节点利用率低于阈值（默认 50%）持续 10 分钟后缩容。
- **Node Group**：定义节点池的大小范围（min/max）和实例类型。
- **Expander**：扩容策略（random/most-pods/least-waste/priority）。

## 使用场景与最佳实践

- 云环境集群必须配置 Cluster Autoscaler 实现成本优化。
- 为不同工作负载定义不同的 Node Group（GPU/CPU/大内存）。
- 使用 PDB（PodDisruptionBudget）保护关键 Pod 不被驱逐。
- 设置合理的 `--scale-down-delay-after-add` 避免频繁扩缩。
- 配合 Karpenter（AWS）获得更灵活的节点供应。

## 参考链接

- [Cluster Autoscaler](https://github.com/kubernetes/autoscaler/tree/master/cluster-autoscaler)

## Related

- [[domain-17-system-foundation/知识字典/scheduling/hpa.md|HPA]]
- [[domain-17-system-foundation/知识字典/scheduling/vpa.md|VPA]]
- [[domain-17-system-foundation/知识字典/scheduling/keda.md|KEDA]]
- [[domain-17-system-foundation/知识字典/operations/pdb.md|PDB]]
- [[domain-17-system-foundation/知识字典/fundamentals/node.md|Node]]


<!-- risk-assessed -->
