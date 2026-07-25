---
title: Litmus 混沌工程
description: LitmusChaos 是 CNCF 孵化项目，提供 Kubernetes 原生的混沌工程平台，内置 300+ 预定义混沌实验，支持通过
  ChaosCenter...
summary: LitmusChaos 是 CNCF 孵化项目，提供 Kubernetes 原生的混沌工程平台，内置 300+ 预定义混沌实验，支持通过 ChaosCenter...
category: dictionary
tags:
- k8s
- glossary
- operations
- chaos-engineering
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Litmus 混沌工程 是什么
- Litmus 详解
trigger_keywords:
- Litmus 混沌工程
- Litmus
- dictionary
prerequisites:
- kubernetes
---



# Litmus 混沌工程（Litmus）

## 概述

LitmusChaos 是 CNCF 孵化项目，提供 Kubernetes 原生的混沌工程平台，内置 300+ 预定义混沌实验，支持通过 ChaosCenter 进行集中管理和可观测性。

## 核心概念/原理

- **Kubernetes 原生**：以 CRD 方式定义混沌实验（ChaosExperiment/ChaosEngine/ChaosResult）
- **300+ 实验**：ChaosHub 提供大量预定义实验（Pod/Network/Node/DNS/Kafka 等）
- **ChaosCenter**：Web UI 集中管理实验编排、调度和结果分析
- **CNCF 孵化**：活跃的开源混沌工程社区

## 关键机制或特性

- 实验编排：多步骤串/并行组合混沌实验
- 弹性探针（Probes）：HTTP/CMD/Prometheus/Continuous 验证
- GitOps 集成：通过 Argo 管理混沌实验
- 混沌实验评分（Resilience Score）量化系统弹性
- 支持 Argo Workflows 编排复杂故障注入流程
- 与 Prometheus/Grafana 集成可视化

## 使用场景与最佳实践

- 生产环境弹性验证
- CI/CD Pipeline 中的自动化弹性测试
- 故障演练和红蓝对抗
- 新服务上线前的 Chaos Day
- SLO 验证和容量规划

## 参考链接

- https://litmuschaos.io/
- https://github.com/litmuschaos/litmus

## Related

- [[17-系统基础/06-知识字典/operations/chaos-engineering.md|混沌工程]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]
- [[17-系统基础/06-知识字典/operations/argo.md|Argo]]
