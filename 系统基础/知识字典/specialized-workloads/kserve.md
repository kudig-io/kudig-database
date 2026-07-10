---
title: KServe
description: KServe（原 KFServing）是 CNCF 孵化项目，为 Kubernetes 提供标准化的机器学习模型推理（Inference）服务。它支持自动扩缩容...
summary: KServe（原 KFServing）是 CNCF 孵化项目，为 Kubernetes 提供标准化的机器学习模型推理（Inference）服务。它支持自动扩缩容...
category: dictionary
tags:
- k8s
- glossary
- kserve
- ml
- inference
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
- KServe 是什么
- KServe 详解
trigger_keywords:
- KServe
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KServe

> **英文名**: KServe

## 概述

KServe（原 KFServing）是 CNCF 孵化项目，为 Kubernetes 提供标准化的机器学习模型推理（Inference）服务。它支持自动扩缩容、金丝雀发布和多模型服务，是 ML 部署的标准方案。

## 核心概念/原理

### 核心概念

- **InferenceService**：模型服务的核心 CRD。
- **Predictor**：模型推理器（支持 TensorFlow/PyTorch/SKLearn/XGBoost 等）。
- **Transformer**：请求/响应的预处理/后处理。
- **Explainer**：模型可解释性服务（Alibi/AIX）。

### 特性

| 特性 | 说明 |
|------|------|
| 自动扩缩 | 缩到零（Scale-to-Zero） |
| 金丝雀发布 | 模型版本的渐进式切换 |
| 多模型 | ModelMesh 支持大量模型共享资源 |
| GPU 调度 | 自动管理 GPU 资源分配 |

## 关键机制或特性

- **Serverless 模式**：基于 Knative，支持缩到零降低成本。
- **RawDeployment 模式**：不依赖 Knative，适合简单场景。
- **ModelMesh**：在少量 Pod 中加载大量模型，适合大规模模型服务。
- **V2 协议**：标准化的推理 API（Predict/Explain）。
- 支持 ONNX Runtime、Triton 等多种推理引擎。

## 使用场景与最佳实践

- ML 模型上线使用 KServe 替代自建的推理服务。
- 配合 Kubeflow 实现训练到部署的全自动化。
- 使用金丝雀发布逐步切换新模型版本。
- 低成本场景启用 Scale-to-Zero。
- 大规模模型服务使用 ModelMesh 优化资源利用。

## 参考链接

- [KServe Official](https://kserve.github.io/website/)

## Related

- [[系统基础/知识字典/specialized-workloads/kubeflow.md|Kubeflow]]
- [[系统基础/知识字典/specialized-workloads/knative.md|Knative]]
- [[系统基础/知识字典/workloads/deployment.md|Deployment]]
- [[系统基础/知识字典/scheduling/hpa.md|HPA]]
- [[系统基础/知识字典/scheduling/keda.md|KEDA]]


<!-- risk-assessed -->
