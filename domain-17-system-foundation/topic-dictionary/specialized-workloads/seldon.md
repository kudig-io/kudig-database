---
title: Seldon
description: 'Seldon 是 ML 模型部署和推理管理平台，提供 Seldon Core（K8s 原生推理引擎）和 Seldon Deploy（企业级 ML 部署管理）。它...'
category: dictionary
tags:
- k8s
- glossary
- seldon
- ml
- inference
- mlops
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Seldon 是什么
- Seldon 详解
trigger_keywords:
- Seldon
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Seldon

> **英文名**: Seldon

## 概述

Seldon 是 ML 模型部署和推理管理平台，提供 Seldon Core（K8s 原生推理引擎）和 Seldon Deploy（企业级 ML 部署管理）。它支持多框架模型的可扩展部署和 A/B 测试。

## 核心概念/原理

### 核心概念

- **SeldonDeployment**：模型推理服务的 CRD。
- **Inference Graph**：多模型编排（串行/并行/路由/组合）。
- **Pre-packaged Servers**：预构建的模型服务（SKLearn/XGBoost/TensorFlow 等）。

### 与 KServe 对比

| 特性 | Seldon | KServe |
|------|--------|--------|
| 成熟度 | 成熟 | CNCF 孵化 |
| 编排 | 丰富的 Graph | 简单 |
| 企业功能 | Seldon Deploy | 开源 |

## 关键机制或特性

- **推理图**：组合多个模型（预处理→推理→后处理）。
- **A/B 测试**：基于权重的流量分配到不同模型版本。
- **Metrics**：内置推理延迟和请求量指标。
- **Explainer**：模型可解释性集成（Alibi）。
- 支持自定义 Python/Java 推理容器。

## 使用场景与最佳实践

- 需要复杂模型编排（多模型组合）时选择 Seldon。
- 使用 Inference Graph 构建 ML Pipeline 的推理阶段。
- 配合 Seldon Deploy 管理大规模模型部署。
- 使用 A/B 测试验证新模型版本的效果。
- 考虑 KServe 作为轻量替代方案。

## 参考链接

- [Seldon Official](https://www.seldon.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve.md|KServe]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kubeflow.md|Kubeflow]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/ray.md|Ray]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
