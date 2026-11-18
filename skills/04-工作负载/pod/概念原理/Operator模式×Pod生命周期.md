---
title: Operator 模式 × Pod 生命周期
description: '[[wiki/22-概念/01-核心架构/operator-pattern.md|operator pattern]] 描述 CRD + 自定义控制器的扩展模式，[[wiki/22-概念/02-工作负载/pod-lifecycle.md|pod
  lifecycle]] 描述 Pod 的标准状态机（Pending→Running→Terminating）。两者的交叉点是**有状态应用**：StatefulSet
  管理了 Pod 的有序创建和稳定身份，但数据库的备份、恢复、版本升级、故障转移等业务逻辑完全超出了标'
summary: '[[wiki/22-概念/01-核心架构/operator-pattern.md|operator pattern]] 描述 CRD + 自定义控制器的扩展模式，[[wiki/22-概念/02-工作负载/pod-lifecycle.md|pod
  lifecycle]] 描述 Pod 的标准状态机（Pending→Running→Terminating）。两者的交叉点是**有状态应用**：StatefulSet
  管理了 Pod 的有...'
category: synthesis
tags:
- k8s
- operator
- pod
- lifecycle
- stateful
- etcd
- redis
- postgresql
- statefulset
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Operator 模式 × Pod 生命周期 是什么
- 如何 Operator 模式 × Pod 生命周期
trigger_keywords:
- Operator
- 模式
- Pod
- 生命周期
prerequisites:
- kubectl-basics
- etcd-basics
- redis-basics
relationships:
- target: '[[wiki/23-实体/02-K8s核心组件/etcd.md]]'
  type: uses
- target: '[[wiki/23-实体/02-K8s核心组件/kubernetes.md]]'
  type: uses
- target: '[[wiki/23-实体/12-数据与消息/cloudnativepg.md]]'
  type: related_to
- target: '[[wiki/17-系统基础/06-知识字典/workloads/cronjob.md]]'
  type: related_to
- target: '[[wiki/23-实体/08-交付与制品/distribution.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/22-概念/11-交叉分析/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]]（本页为重排去重桩，正文见主场）
