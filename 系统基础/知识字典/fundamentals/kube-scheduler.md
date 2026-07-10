---
title: 调度器
description: kube-scheduler 是 Kubernetes 控制平面的默认调度器，负责将新创建的 Pod 分配到最合适的节点上运行。调度决策基于资源需求、亲和性约束...
summary: kube-scheduler 是 Kubernetes 控制平面的默认调度器，负责将新创建的 Pod 分配到最合适的节点上运行。调度决策基于资源需求、亲和性约束...
category: dictionary
tags:
- k8s
- glossary
- scheduler
- control-plane
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

kube-scheduler 是 Kubernetes 控制平面的默认调度器，负责将新创建的 Pod 分配到最合适的节点上运行。调度决策基于资源需求、亲和性约束、污点容忍等多种因素。

## 核心概念/原理

### 调度流程

调度器采用两阶段流程：

1. **过滤（Filtering）**：排除不满足 Pod 约束的节点（资源不足、污点不匹配、节点亲和性不满足等）。
2. **打分（Scoring）**：对通过过滤的节点评分，选择得分最高的节点。

### 调度框架

kube-scheduler 基于可插拔的 Scheduling Framework 架构，支持自定义 Filter、Score、Bind 等扩展点。

## 关键机制或特性

- 内置 Filter 插件：NodeResourcesFit、NodeAffinity、TaintToleration、PodTopologySpread 等。
- 内置 Score 插件：InterPodAffinity、LeastRequestedPower、BalancedAllocation 等。
- 支持 **调度器扩展配置（KubeSchedulerConfiguration）** 自定义调度行为。
- 支持多调度器共存，通过 `schedulerName` 字段指定 Pod 使用的调度器。

## 使用场景与最佳实践

- 为关键工作负载配置 Pod Priority，确保高优先级 Pod 能够被调度。
- 使用 topologySpreadConstraints 实现 Pod 跨可用区均匀分布。
- 在大型集群中调整 `percentageOfNodesToScore` 平衡调度速度和准确性。
- 监控 pending Pod 数量和调度延迟指标。

## 参考链接

- [kube-scheduler - Official Documentation](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-scheduler/)

## Related

[[实体/scheduling-terms.md|调度术语参考]]


<!-- risk-assessed -->
