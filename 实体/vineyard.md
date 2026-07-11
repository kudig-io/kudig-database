---
title: Vineyard
description: '## 概述'
summary: 'Vineyard 是一个内存中的不可变数据管理器，为大数据和 AI/ML 工作流提供零拷贝数据共享。它通过共享内存机制在同一节点上的不同计算引擎（如 Spark、PyTorch、Dask、GraphScope）之间实现高效数据传递，避免了传统方式中序列化/反序列化和磁盘 IO 的开销，可将数据流水线的端到端性能提升数倍。'
category: entities
tags:
- k8s
- cncf
- data
- vineyard
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Vineyard 是什么
- 如何 Vineyard
trigger_keywords:
- Vineyard
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Vineyard

> **CNCF 状态**: Sandbox | **类别**: Data | **主要语言**: C++, Python

## 概述

Vineyard（葡萄藤）是一个 CNCF 沙箱项目，由阿里巴巴和北京大学联合开发，是一个面向数据密集型计算的共享内存对象存储系统。它专门解决分布式计算环境中大数据对象在不同进程和任务间的高效共享问题。Vineyard 通过零拷贝的共享内存机制，避免了 Python 对象在进程间传输时的序列化/反序列化开销，特别适合大规模机器学习和图计算场景。

## Key Features（核心能力）

- **共享内存对象存储**：基于 IPC 共享内存实现进程间零拷贝数据共享
- **Python 原生集成**：支持 NumPy、pandas、PyTorch Tensor 等常用数据结构
- **分布式架构**：跨节点的对象管理和迁移
- **K8s 集成**：通过 Vineyard Operator 管理 K8s 上的分布式 Vineyard 实例
- **Plasma 兼容**：与 Apache Arrow Plasma 格式兼容
- **多种后端支持**：可将对象溢出到磁盘或对象存储

## 架构与工作原理

Vineyard 架构包含 Vineyardd（守护进程，管理每个节点上的共享内存段）、IPC 层（本地进程通过 Unix Domain Socket 访问共享内存）、RPC 层（跨节点通信和对象迁移）。对象以 Blob（二进制数据块）和 Meta（元数据描述）两级结构组织。Vineyard Operator 在 K8s 上以 DaemonSet 方式部署 Vineyardd 到每个计算节点，为 Pod 提供共享内存卷。

## K8s 集成

Vineyard 通过 Vineyard Operator 与 Kubernetes 集成。Operator 以 DaemonSet 方式在每个计算节点部署 vineyardd 守护进程。计算 Pod 通过 Device Plugin 挂载 Vineyard 共享内存段。Vineyard CRD 定义集群配置和对象恢复策略。与 Dask、Ray、Kubeflow 等分布式计算框架集成时，Vineyard 作为中间数据存储层加速计算任务间的数据交换。

## 生产用例

- **分布式 ML 训练**：训练任务间共享大型数据集和模型参数
- **图计算**：大规模图数据的跨进程高效共享
- **数据管道**：ETL 流水线中数据处理任务间的数据传递
- **科学计算**：大规模数值模拟数据的实时分析

## 安装与快速开始

```bash
# Python SDK
pip install vineyard

# K8s Operator
helm repo add vineyard https://vineyard.oss-ap-southeast-1.aliyuncs.com/charts/
helm install vineyardd vineyard/vineyardd -n vineyard --create-namespace
```

## 对比替代方案

相比 Redis/Memcached（通用缓存），Vineyard 专为大数据对象的零拷贝共享设计，避免了序列化开销。相比 Plasma（Apache Arrow），Vineyard 提供分布式支持和 K8s 集成。

## Related

- [[hami]] — HAMI
- [[open-policy-containers]] — [[实体/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[werf]] — werf
- [[dalec]] — Dalec
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- vineyard
- storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
