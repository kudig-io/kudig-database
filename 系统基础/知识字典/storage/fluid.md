---
title: Fluid 数据编排
description: Fluid 是 CNCF Sandbox 项目，为 Kubernetes 上的 AI/ML 和大数据工作负载提供数据编排和加速能力，通过
  Dataset + R...
summary: Fluid 是 CNCF Sandbox 项目，为 Kubernetes 上的 AI/ML 和大数据工作负载提供数据编排和加速能力，通过 Dataset
  + R...
category: dictionary
tags:
- k8s
- glossary
- storage
- ai-ml
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
- Fluid 数据编排 是什么
- Fluid 详解
trigger_keywords:
- Fluid 数据编排
- Fluid
- dictionary
prerequisites:
- kubernetes
---



# Fluid 数据编排（Fluid）

## 概述

Fluid 是 CNCF Sandbox 项目，为 Kubernetes 上的 AI/ML 和大数据工作负载提供数据编排和加速能力，通过 Dataset + Runtime 抽象统一管理异构存储系统的数据访问。

## 核心概念/原理

- **数据抽象**：Dataset CRD 统一描述数据的元数据和访问方式
- **运行时抽象**：Runtime CRD 管理数据缓存引擎（Alluxio/JindoFS/JuiceFS/GooseFS）
- **数据感知调度**：将计算任务调度到数据所在位置
- **CNCF Sandbox**：阿里巴巴开源

## 关键机制或特性

- 支持多种缓存 Runtime（Alluxio、JindoFS、JuiceFS、GooseFS、Vineyard）
- 数据预热（Data Prefetching）
- 弹性数据集（Elastic Dataset）动态扩缩
- 与 Spark/TensorFlow/PyTorch Operator 集成
- 数据迁移和复制
- 跨命名空间数据共享

## 使用场景与最佳实践

- AI 训练任务的数据加速
- 大数据分析（Spark/Flink）的数据本地化
- 多云/混合存储的统一访问层
- 训练数据的预热和缓存管理
- 大规模数据集的弹性管理

## 参考链接

- https://fluid-cloudnative.github.io/
- https://github.com/fluid-cloudnative/fluid

## Related

- [[系统基础/topic-dictionary/specialized-workloads/kubeflow.md|Kubeflow]]
- [[系统基础/topic-dictionary/storage/ceph.md|Ceph]]
- [[系统基础/topic-dictionary/storage/minio.md|MinIO]]
