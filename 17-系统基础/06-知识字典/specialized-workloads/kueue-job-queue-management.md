---
title: Kueue 作业队列与准入控制
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- job
- gpu
- nvidia
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kueue 作业队列与准入控制 是什么
- 如何 Kueue 作业队列与准入控制
trigger_keywords:
- Kueue
- 作业队列与准入控制
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kueue 作业队列与准入控制

## 概述

**Kueue** 是 [[Kubernetes|Kubernetes]] 官方推出的**作业队列与集群级资源配额管理系统**，专门解决 AI/ML、批处理（Batch）和高性能计算（HPC）场景下的资源争抢与调度公平性问题。在 2026 年的 AI 基础设施实践中，Kueue 已成为管理 GPU 集群稀缺资源的标配工具。

## 核心概念/原理

### 1. 队列化准入控制（Queue-Based Admission）

传统 Kubernetes 中，工作负载一旦创建即尝试占用资源，容易导致：
- 资源碎片化和争抢
- 重要任务被低优先级任务阻塞
- GPU 利用率低下（各团队 hoarding 资源）

Kueue 引入了**队列化准入控制**：
- 工作负载先进入队列等待
- 当集群有足够可用资源时，Kueue 才将其**准入（Admit）**到调度器
- 未准入的作业不消耗任何实际资源

### 2. 核心资源模型

#### ClusterQueue
代表集群级别的共享资源池，定义了可用资源的总量和借用策略。一个 ClusterQueue 可以关联多个本地队列。

#### LocalQueue
部署在特定 Namespace 中，供用户提交作业。LocalQueue 指向一个 ClusterQueue，实现多租户隔离。

#### Workload
Kueue 为每个待调度的 Job/PodGroup 创建一个 Workload 对象，用于跟踪排队状态、资源需求和优先级。

#### ResourceFlavor
定义了不同资源"风味"的节点池，例如：
- `nvidia-a100`：配备 A100 GPU 的节点池
- `nvidia-h100`：配备 H100 GPU 的节点池
- `spot-cpu`：可抢占的低成本 CPU 节点池

### 3. 公平共享与抢占

Kueue 支持 **Cohort（队列组）** 机制：
- 多个 ClusterQueue 可以组成一个 Cohort
- 当某个队列空闲时，其他队列可以**借用**其剩余配额
- 当原队列需要资源时，可以**抢占**被借用的工作负载（前提是目标作业支持被抢占）

### 4. 与现有调度器的集成

Kueue 位于 **API Server 与默认调度器之间**：
- 拦截 Job/Deployment 的创建请求
- 管理准入决策
- 一旦准入，标准的 Kubernetes 调度器负责具体的节点放置
- 可无缝配合 [[Volcano|Volcano]]、Scheduler Plugins 等高级调度器使用

## 关键机制或特性

| 机制 | 说明 | 收益 |
|------|------|------|
| 队列准入 | 作业先排队，资源满足后再创建 Pod | 防止资源碎片和争抢 |
| 公平共享 | 按配额比例分配资源，支持借用和抢占 | 提升整体利用率 30%–50% |
| 资源风味 | 为不同硬件类型定义独立队列 | 实现异构 GPU 的精细化管理 |
| 抢占与回填 | 高优先级任务可抢占低优先级任务的资源 | 保障关键 SLA |
| Spot/Preemptible 队列 | 将可中断作业路由到低成本可抢占实例 | 降低训练成本 50%–80% |

## 使用场景

1. **企业级 GPU 共享平台**：多个研究团队共享同一 GPU 集群，通过 Kueue 实现配额隔离和公平调度
2. **大模型训练作业管理**：将分布式训练作业排队，确保其获得所需的整卡 GPU 和 NVLink 拓扑
3. **混合批处理与推理**：批处理训练任务进入低优先级队列，在线推理服务进入高优先级队列
4. ** Spot 实例训练**：为支持 checkpointing 的训练作业配置 Spot ResourceFlavor，大幅降低算力成本

## 最佳实践/注意事项

- **从队列配额开始设计**：先定义各团队的 ClusterQueue 配额，再允许用户提交作业
- **配合 [[17-系统基础/06-知识字典/scheduling/gang-scheduling.md|Gang Scheduling]] 使用**：分布式训练作业应启用 Volcano/Kube-Batch 的 Gang Scheduling，与 Kueue 协同防止资源死锁
- **Checkpoint 是 Spot 实例的前提**：只有具备完善 checkpoint 机制的作业才能安全使用可抢占资源
- **设置合理的抢占策略**：避免频繁抢占导致训练任务反复重启，可配置抢占宽限期（Preemption Grace Period）
- **监控队列深度和等待时间**：核心指标包括队列中的 Workload 数量、平均等待时间、准入率
- **资源借用需谨慎**：跨队列借用虽提升利用率，但可能引发跨团队资源纠纷，需明确 SLO

## 参考链接

- [Kueue Official Documentation](https://kueue.sigs.k8s.io/)
- [Kueue GitHub Repository](https://github.com/kubernetes-sigs/kueue)
- [Kubernetes Scheduling for AI Workloads](https://kubernetes.io/docs/concepts/scheduling-eviction/)
- [CIO - Kubernetes GPU Utilization Best Practices](https://www.cio.com/article/4152554/how-kubernetes-is-finally-solving-the-gpu-utilization-crisis-to-save-your-ai-budget.html)

## Related

- [[17-系统基础/06-知识字典/workloads/pod.md|Pod]]
- [[17-系统基础/06-知识字典/fundamentals/container.md|Container]]
- [[17-系统基础/06-知识字典/fundamentals/node.md|Node]]
- [[17-系统基础/06-知识字典/fundamentals/namespace.md|Namespace]]
- [[17-系统基础/06-知识字典/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
