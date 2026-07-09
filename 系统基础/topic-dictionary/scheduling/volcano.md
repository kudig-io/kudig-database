---
title: Volcano 批处理调度
description: Volcano 是 CNCF 孵化项目，专为 Kubernetes 上的批处理、AI/ML、HPC 等高性能计算工作负载设计的批量调度系统，弥补原生调度器在
  g...
summary: Volcano 是 CNCF 孵化项目，专为 Kubernetes 上的批处理、AI/ML、HPC 等高性能计算工作负载设计的批量调度系统，弥补原生调度器在
  g...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- batch
- ai-ml
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volcano 批处理调度 是什么
- Volcano 详解
trigger_keywords:
- Volcano 批处理调度
- Volcano
- dictionary
prerequisites:
- kubernetes
---



# Volcano 批处理调度（Volcano）

## 概述

Volcano 是 CNCF 孵化项目，专为 Kubernetes 上的批处理、AI/ML、HPC 等高性能计算工作负载设计的批量调度系统，弥补原生调度器在 gang-scheduling 和公平共享方面的不足。

## 核心概念/原理

- **Gang Scheduling**：保证一组 Pod 全部调度成功或全部不调度（all-or-nothing）
- **公平共享**：基于 Queue 的多租户资源公平分配
- **AI/ML 优化**：为 TensorFlow、PyTorch、MPI 等训练框架优化调度
- **CNCF 孵化**：华为开源，AI/ML 领域广泛使用

## 关键机制或特性

- Queue CRD 定义资源配额和优先级
- Job CRD 定义批量任务（gang-scheduling + task 类型）
- 抢占（Preemption）和回填（Backfill）策略
- Binpack 插件优化 GPU 资源利用率
- 支持 MPI Operator 和 TensorFlow Operator
- 与 Kubeflow 深度集成

## 使用场景与最佳实践

- AI/ML 分布式训练任务调度
- 大数据批处理（Spark、Flink）
- HPC 高性能计算
- 多租户 GPU 集群的公平调度
- 需要 Gang Scheduling 的任何工作负载

## 参考链接

- https://volcano.sh/
- https://github.com/volcano-sh/volcano

## Related

- [[系统基础/topic-dictionary/scheduling/scheduler.md|Scheduler]]
- [[系统基础/topic-dictionary/specialized-workloads/kubeflow.md|Kubeflow]]
- [[系统基础/topic-dictionary/specialized-workloads/ray.md|Ray]]
