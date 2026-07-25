---
title: Armada 批量调度
description: Armada 是 G-Research 开源的 CNCF Sandbox 项目，专为大规模批量工作负载设计的多集群调度系统，管理跨多个 K8s
  集群的队列和作业...
summary: Armada 是 G-Research 开源的 CNCF Sandbox 项目，专为大规模批量工作负载设计的多集群调度系统，管理跨多个 K8s 集群的队列和作业...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- batch
- multi-cluster
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Armada 批量调度 是什么
- Armada 详解
trigger_keywords:
- Armada 批量调度
- Armada
- dictionary
prerequisites:
- kubernetes
---



# Armada 批量调度（Armada）

## 概述

Armada 是 G-Research 开源的 CNCF Sandbox 项目，专为大规模批量工作负载设计的多集群调度系统，管理跨多个 K8s 集群的队列和作业优先级。

## 核心概念/原理

- **多集群批量调度**：跨多个 K8s 集群调度批处理作业
- **队列管理**：多级队列和优先级抢占
- **CNCF Sandbox**：G-Research（量化对冲基金）主导
- **大规模**：支撑数十万核的批量计算

## 关键机制或特性

- JobSet CRD 定义批量作业集
- Queue CRD 多级队列管理
- 优先级和抢占策略
- 跨集群作业分发
- 资源公平共享（Fair Share）
- 作业状态聚合
- Lookout UI 作业监控

## 使用场景与最佳实践

- 量化研究的批量计算
- AI 训练任务的多集群调度
- 大规模数据处理 Pipeline
- 多团队的计算资源公平分配
- HPC 工作负载的 K8s 管理

## 参考链接

- https://armadaproject.io/
- https://github.com/armadaproject/armada

## Related

- [[17-系统基础/06-知识字典/scheduling/volcano.md|Volcano]]
- [[17-系统基础/06-知识字典/scheduling/koordinator.md|Koordinator]]
- [[17-系统基础/06-知识字典/platform-engineering/karmada.md|Karmada]]
