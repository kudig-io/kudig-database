---
title: Kubeflow
description: Kubeflow 是 CNCF 孵化项目，为 Kubernetes 上的机器学习工作负载提供完整的工具链。它涵盖 ML Pipeline、Notebook、超参...
summary: Kubeflow 是 CNCF 孵化项目，为 Kubernetes 上的机器学习工作负载提供完整的工具链。它涵盖 ML Pipeline、Notebook、超参...
category: dictionary
tags:
- k8s
- glossary
- kubeflow
- ml
- ai
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubeflow 是什么
- Kubeflow 详解
trigger_keywords:
- Kubeflow
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubeflow

> **英文名**: Kubeflow

## 概述

Kubeflow 是 CNCF 孵化项目，为 Kubernetes 上的机器学习工作负载提供完整的工具链。它涵盖 ML Pipeline、Notebook、超参调优、模型训练和部署的全生命周期管理。

## 核心概念/原理

### 核心组件

| 组件 | 功能 |
|------|------|
| Kubeflow Pipelines | ML 工作流编排（基于 Argo） |
| Notebooks | Jupyter Notebook 管理 |
| Katib | 超参数调优和神经架构搜索 |
| Training Operators | 分布式训练（TF/PyTorch/MXNet） |
| KServe | 模型推理服务（独立项目） |

### ML 工作流

```
数据准备 → 特征工程 → 模型训练 → 超参调优 → 模型评估 → 部署服务
  (Pipeline)  (Notebook)  (Training)  (Katib)   (Pipeline)  (KServe)
```

## 关键机制或特性

- **Pipeline SDK**：Python SDK 定义 ML 工作流步骤。
- **分布式训练**：PyTorchJob/TFJob 管理多 GPU/多节点训练。
- **资源调度**：GPU 调度、优先级队列、资源隔离。
- **模型注册**：版本化管理训练好的模型。
- **Experiment Tracking**：跟踪实验参数和指标。

## 使用场景与最佳实践

- 需要标准化 ML 工作流时引入 Kubeflow Pipelines。
- GPU 训练任务使用 Training Operators 管理。
- 使用 Katib 自动化超参搜索。
- 配合 KServe 实现模型的在线推理服务。
- 注意 Kubeflow 的资源开销，小型团队可考虑轻量替代方案。

## 参考链接

- [Kubeflow Official](https://www.kubeflow.org/)

## Related

- [[domain-17-system-foundation/知识字典/specialized-workloads/kserve.md|KServe]]
- [[domain-17-system-foundation/知识字典/workloads/job.md|Job]]
- [[domain-17-system-foundation/知识字典/scheduling/resource-request.md|Resource Request]]
- [[domain-17-system-foundation/知识字典/operations/argo.md|Argo]]
- [[domain-17-system-foundation/知识字典/platform-engineering/operator-pattern.md|Operator Pattern]]


<!-- risk-assessed -->
