---
title: GPU 资源管理与分区技术
description: '# GPU 资源管理与分区技术'
category: dictionary
tags:
- k8s
- glossary
- terminology
- gpu
- cuda
- nvidia
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GPU 资源管理与分区技术 是什么
- 如何 GPU 资源管理与分区技术
trigger_keywords:
- GPU
- 资源管理与分区技术
- dictionary
title_en: Gpu Resource Management And Partitioning
---


# GPU 资源管理与分区技术

## 概述

在 Kubernetes 上运行 AI/ML 工作负载时，GPU 是最昂贵且最稀缺的资源。2026 年的行业最佳实践要求平台团队不仅要将 GPU 暴露给 Pod，还需通过**分区（Partitioning）、共享（Sharing）、拓扑感知调度（Topology-Aware Scheduling）**等手段，将 GPU 利用率从传统的 13%–40% 提升至 70% 以上，从而显著降低 AI 基础设施成本。

## 核心概念/原理

### 1. Device Plugin 机制

Kubernetes 通过 **Device Plugin** 框架将 GPU、TPU 等硬件资源以 `nvidia.com/gpu`、`amd.com/gpu` 等形式注册到节点可分配资源池中。调度器在分配 Pod 时会检查节点上是否有足够的设备资源。

### 2. GPU 分区技术

#### MIG（Multi-Instance GPU）
- **硬件级隔离**：NVIDIA A100/H100 支持将单张物理 GPU 切分为最多 7 个独立实例
- 每个 MIG 实例拥有独立的显存、CUDA 核心和内存带宽
- 适用于**多租户推理场景**，提供强隔离保障

#### Time-Slicing（时间切片）
- 通过 NVIDIA Device Plugin 的 time-slicing 配置，将单张 GPU 虚拟化为多个逻辑 GPU
- 多个 Pod 按时间片轮询使用 GPU，**无硬件隔离**，存在性能抖动
- 适用于**开发测试、低优先级批处理**等容忍延迟抖动的场景

#### NVIDIA MPS（Multi-Process Service）
- 允许多个 CUDA 进程共享同一张 GPU 的上下文
- 比时间切片开销更低，但无显存隔离
- 适合**同租户内部的多推理服务共享**

### 3. 拓扑感知调度

分布式训练任务对 GPU 间通信带宽极为敏感。平台应通过拓扑感知调度将相关 Pod 调度到：
- 同一 NUMA 节点
- 通过 NVLink 或 NVSwitch 直连的 GPU 组
- 同一 InfiniBand 域内

这可以显著减少跨网络边界的数据传输延迟，提升训练吞吐量。

### 4. 动态资源分配（DRA）

Kubernetes 1.26+ 引入的 **Dynamic Resource Allocation (DRA)** 提供了比 Device Plugin 更灵活的资源模型。DRA 支持：
- 定义自定义资源分配语义
- 多节点 NVLink Domain 抽象
- 更细粒度的调度决策

## 关键机制或特性

| 技术 | 隔离级别 | 最佳场景 | 成本效益 |
|------|----------|----------|----------|
| MIG | 硬件级强隔离 | 生产推理、多租户 | ⭐⭐⭐⭐⭐ |
| MPS | 软件级弱隔离 | 同租户多服务共享 | ⭐⭐⭐⭐ |
| Time-Slicing | 软件级无隔离 | 开发测试、实验 | ⭐⭐⭐ |
| DRA | 自定义分配策略 | 大规模分布式训练 | ⭐⭐⭐⭐⭐ |

### GPU 利用率提升路径

根据 2026 年 CNCF 生产案例研究：
- **基线利用率**：约 13%（每任务独占整卡，大量空闲）
- **队列化准入控制**：提升至 30%–50%
- **MIG + 时间切片混合分层**：提升至 60%+
- **拓扑感知 + 连续批处理**：提升至 90%+

## 使用场景

1. **大模型分布式训练**：使用 DRA + 拓扑感知调度，将多机多卡任务调度到 NVLink/InfiniBand 最优拓扑
2. **多租户推理平台**：为不同业务线分配独立的 MIG 实例，确保 SLA 互不干扰
3. **GPU 共享开发环境**：使用时间切片或 MPS，让多个研究员共享同一张 GPU 进行实验
4. **混合负载集群**：训练任务使用整卡或 MIG，推理任务使用 MIG 切片，开发任务使用时间切片

## 最佳实践/注意事项

- **分层策略**：生产推理用 MIG，内部开发用 Time-Slicing，大规模训练用整卡 + DRA
- **显存是硬限制**：GPU 算力可以共享，但显存不可超分，超出即 OOM
- **队列化准入控制**：使用 Kueue 等队列系统替代直接抢占，实现公平共享和配额管理
- **Checkpoint 与 Spot 实例结合**：训练任务必须具备 checkpoint 能力，才能安全运行在可抢占 GPU 实例上，降低成本 50%–80%
- **监控 GPU 利用率**：不仅监控 GPU 使用率，还需监控显存占用、NVLink 带宽、温度与功耗
- **避免跨可用区调度分布式训练**：跨 AZ 的网络延迟会严重拖累训练效率

## 参考链接

- [NVIDIA MIG User Guide](https://docs.nvidia.com/datacenter/tesla/mig-user-guide/)
- [NVIDIA Device Plugin for Kubernetes](https://github.com/NVIDIA/k8s-device-plugin)
- [Kubernetes Dynamic Resource Allocation](https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/)
- [CIO - How Kubernetes is solving the GPU utilization crisis](https://www.cio.com/article/4152554/how-kubernetes-is-finally-solving-the-gpu-utilization-crisis-to-save-your-ai-budget.html)
