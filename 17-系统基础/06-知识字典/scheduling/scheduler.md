---
title: 调度器
description: kube-scheduler 是 Kubernetes 控制平面组件，负责将新创建的 Pod 分配到最合适的节点上。它通过一系列过滤（Filtering）和打分...
summary: kube-scheduler 是 Kubernetes 控制平面组件，负责将新创建的 Pod 分配到最合适的节点上。它通过一系列过滤（Filtering）和打分...
category: dictionary
tags:
- k8s
- glossary
- scheduler
- control-plane
- scheduling
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 调度器 是什么
- kube-scheduler 详解
trigger_keywords:
- 调度器
- kube-scheduler
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 调度器

> **英文名**: kube-scheduler

## 概述

kube-scheduler 是 Kubernetes 控制平面组件，负责将新创建的 Pod 分配到最合适的节点上。它通过一系列过滤（Filtering）和打分（Scoring）算法实现智能调度决策。

## 核心概念/原理

### 调度流程

1. **Filtering（过滤）**：排除不满足 Pod 要求的节点（资源不足、污点不匹配、亲和性约束等）。
2. **Scoring（打分）**：对剩余节点评分（资源均衡、数据本地性、拓扑分布等）。
3. **Binding（绑定）**：将 Pod 绑定到得分最高的节点。

### 内置调度插件

| 插件 | 阶段 | 功能 |
|------|------|------|
| NodeResourcesFit | Filter + Score | 资源匹配和均衡 |
| TaintToleration | Filter + Score | 污点容忍检查 |
| PodTopologySpread | Filter + Score | 拓扑分布约束 |
| InterPodAffinity | Filter + Score | Pod 亲和/反亲和 |
| VolumeBinding | Filter + Reserve | 存储卷绑定 |

## 关键机制或特性

- 调度器以 Scheduling Framework 架构运行，支持插件化扩展。
- Scheduler Extender 和 Scheduling Plugin 两种扩展方式。
- Priority Class 影响 Pod 的调度优先级和抢占（Preemption）行为。
- 调度器指标通过 `/metrics` 端点暴露（scheduling_duration、binding_duration 等）。

## 使用场景与最佳实践

- 使用 `--percentage-of-nodes-to-score` 调优大规模集群的调度性能。
- 自定义调度需求优先使用 Scheduling Plugin 而非 Extender。
- 为关键工作负载设置 PriorityClass 确保调度优先级。
- 使用 Descheduler 周期性重新平衡集群中的 Pod 分布。

## 参考链接

- [kube-scheduler - Kubernetes Docs](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-scheduler/)

## Related

- [[17-系统基础/06-知识字典/scheduling/affinity.md|Affinity]]
- [[17-系统基础/06-知识字典/scheduling/taint.md|Taint]]
- [[17-系统基础/06-知识字典/scheduling/toleration.md|Toleration]]
- [[17-系统基础/06-知识字典/scheduling/topology-spread-constraints.md|Topology Spread Constraints]]
- [[17-系统基础/06-知识字典/scheduling/resource-request.md|Resource Request]]


<!-- risk-assessed -->
