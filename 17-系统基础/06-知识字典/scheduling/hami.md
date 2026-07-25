---
title: HAMi 异构资源调度
description: HAMi（Heterogeneous AI Computing Middleware）是 CNCF Sandbox 项目，为 Kubernetes
  提供 GPU...
summary: HAMi（Heterogeneous AI Computing Middleware）是 CNCF Sandbox 项目，为 Kubernetes
  提供 GPU...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- gpu
- heterogeneous
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HAMi 异构资源调度 是什么
- HAMi 详解
trigger_keywords:
- HAMi 异构资源调度
- HAMi
- dictionary
prerequisites:
- kubernetes
---



# HAMi 异构资源调度（HAMi）

## 概述

HAMi（Heterogeneous AI Computing Middleware）是 CNCF Sandbox 项目，为 Kubernetes 提供 GPU/NPU/DCU 等异构计算资源的细粒度共享和调度，解决 AI 工作负载的资源碎片化问题。

## 核心概念/原理

- **异构调度**：统一管理 GPU/NPU/DCU/RDMA 等异构资源
- **GPU 共享**：GPU 显存和算力的细粒度切分
- **CNCF Sandbox**：中国移动等联合推动
- **AI 优化**：专为 AI/ML 工作负载设计

## 关键机制或特性

- 虚拟 GPU（vGPU）切分（1/100 精度）
- 支持 NVIDIA/AMD/华为昇腾/海光 DCU
- GPU 显存隔离和算力隔离
- 资源用量监控和统计
- 与 Volcano 调度器集成
- 支持 MIG（Multi-Instance GPU）

## 使用场景与最佳实践

- AI 训练集群的 GPU 资源共享
- 推理服务的 GPU 细粒度分配
- 多种异构加速卡的统一管理
- GPU 利用率的优化和降本
- 多租户 AI 平台的资源隔离

## 参考链接

- https://github.com/Project-HAMi/HAMi
- https://project-hami.io/

## Related

- [[17-系统基础/06-知识字典/scheduling/koordinator.md|Koordinator]]
- [[17-系统基础/06-知识字典/scheduling/volcano.md|Volcano]]
- [[17-系统基础/06-知识字典/scheduling/kaito.md|KAITO]]
