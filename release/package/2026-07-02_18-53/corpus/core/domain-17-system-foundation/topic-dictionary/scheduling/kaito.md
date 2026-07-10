---
title: KAITO AI 推理调度
description: KAITO（Kubernetes AI Toolchain Operator）是微软开源的 CNCF Sandbox 项目，通过 Operator
  简化 AI/...
summary: KAITO（Kubernetes AI Toolchain Operator）是微软开源的 CNCF Sandbox 项目，通过 Operator
  简化 AI/...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- ai-ml
- inference
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KAITO AI 推理调度 是什么
- KAITO 详解
trigger_keywords:
- KAITO AI 推理调度
- KAITO
- dictionary
prerequisites:
- kubernetes
---



# KAITO AI 推理调度（KAITO）

## 概述

KAITO（Kubernetes AI Toolchain Operator）是微软开源的 CNCF Sandbox 项目，通过 Operator 简化 AI/ML 模型在 Kubernetes 上的部署和推理服务管理，自动化 GPU 资源分配和模型服务。

## 核心概念/原理

- **模型部署**：声明式 CRD 定义 AI 模型推理服务
- **自动化 GPU**：自动选择和配置 GPU 资源
- **CNCF Sandbox**：微软主导
- **预置模型**：内置主流开源模型的优化配置

## 关键机制或特性

- Workspace CRD 定义推理工作空间
- 预置模型模板（LLaMA/Falcon/Mistral/Phi 等）
- 自动 GPU 配置（型号/内存/并发数）
- 推理端点自动暴露
- 模型版本管理和更新
- 与 KEDA 集成的自动扩缩

## 使用场景与最佳实践

- LLM 推理服务的快速部署
- GPU 资源的自动化管理
- AI 模型服务的高可用部署
- 多模型的统一管理平台
- AI 开发团队的自助服务

## 参考链接

- https://github.com/Azure/kaito
- https://kaito.sh/

## Related

- [[domain-17-system-foundation/知识字典/specialized-workloads/kserve.md|KServe]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/ray.md|Ray]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/kubeflow.md|Kubeflow]]
