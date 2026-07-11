---
title: Volcano [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- volcano
- scheduler
- containerd
- harbor
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volcano 是什么
- 如何 Volcano
trigger_keywords:
- Volcano
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Volcano

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Volcano（原 kube-batch）是一个 CNCF 孵化项目，由华为开源，是 Kubernetes 上的高性能批处理工作负载调度器。它专为 AI/ML 训练、大数据分析、HPC（高性能计算）等批处理场景设计，提供了 gang scheduling（成组调度）、公平调度、队列管理、任务依赖等 K8s 默认调度器不具备的高级调度能力。Volcano 已被众多企业用于 GPU 集群管理和大规模 ML 训练平台。

## Key Features（核心能力）

- **Gang Scheduling**：成组调度，确保所有相关 Pod 同时被调度或全部等待
- **Queue 管理**：支持多级队列、权重和抢占，实现公平资源分配
- **任务依赖**：支持 DAG 依赖关系，定义步骤间执行顺序
- **插件化调度算法**：提供多种调度插件（DRF、Binpack、Spread、Topology）
- **GPU 共享**：支持 GPU 细粒度共享和切分，提升 GPU 利用率
- **Job 控制器**：Volcano Job CRD 支持任务重试、生命周期管理

## 架构与工作原理

Volcano 由三个核心组件构成：Volcano Controller 负责管理 Volcano Job CRD 的生命周期，处理任务状态转换；Volcano Scheduler 作为独立调度器，通过调度插件链（Action/Plugin）执行调度决策；Volcano Admission Webhook 负责 API 校验和默认值填充。调度器支持多种调度插件组合，通过 YAML 配置灵活定义调度策略。Volcano 通过 MutatingWebhook 将 Pod 绑定到自身调度器。

## K8s 集成

Volcano 通过 CRD 与 Kubernetes 集成：Volcano Job（vcjob）定义批处理任务，支持 minAvailable、policies、tasks 等高级配置；Volcano Queue 定义资源队列，支持权重和公平调度；Volcano PodGroup 将相关 Pod 组织成调度单元。通过指定 schedulerName: volcano 将 Pod 交给 Volcano 调度器处理。Volcano 复用 K8s 的 Node/PV/PVC 等资源模型。

## 生产用例

- **分布式 ML 训练**：使用 Gang Scheduling 确保 TensorFlow/PyTorch 训练任务的所有 Worker 同时启动
- **Spark/Flink 批处理**：为大数据分析任务提供公平调度和资源排队
- **HPC 计算**：科学计算、基因测序等高性能计算场景
- **CI/CD 并行任务**：大规模并行构建和测试任务调度

## 安装与快速开始

```bash
helm repo add volcano-sh https://volcano-sh.github.io/helm-charts
helm install volcano volcano-sh/volcano -n volcano-system --create-namespace
```

## 对比替代方案

相比 K8s 默认调度器，Volcano 提供了 Gang Scheduling 和高级队列管理能力，专为批处理工作负载优化。相比 Yarn/Mesos，Volcano 原生运行在 K8s 上，可无缝与容器化应用共存。

## Related

- [[08-containerd-multi-tenant]] — [[containerd|containerd]]rd 多租户|containerd 多租户]]租户|多租户]]
- [[harbor]] — Harbor
- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- volcano
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
