---
title: KAITO
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- ai-ml
- kaito
- falco
- crd
- operator
- gpu
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KAITO 是什么
- 如何 KAITO
trigger_keywords:
- KAITO
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# KAITO

> **CNCF 状态**: Sandbox | **类别**: AI/ML | **主要语言**: Go, Python

## 概述

KAITO 是一个 Kubernetes Operator，简化在 Kubernetes 集群上运行 AI/ML 推理和微调工作负载的流程。它自动化了 GPU 节点的配置、模型下载和推理服务部署，使开发者只需指定模型名称即可部署 AI 推理服务。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模型选择**: 根据延迟和资源预算选择合适的模型规模
- **GPU 类型**: 推理用 A10/T4 即可，训练/微调推荐 A100
- **量化**: 使用 GPTQ/AWQ 量化模型减少 GPU 内存需求
- **自动伸缩**: 配合 Karpenter 实现 GPU 节点的自动扩缩容
- **微调数据**: 使用高质量的领域数据进行 LoRA 微调

## 架构定位

在 CNCF 生态中，kaito 属于 **AI/ML** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[falco]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/storage-model|storage-model]]
- [[concepts/secrets-management|secrets-management]]

## Related

- [[kcl]] — KCL (Kusion Configuration Language)
- [[kube-vip]] — kube-vip
- [[kitops]] — KitOps
- [[kairos]] — Kairos
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kaito
- [[entities/cncf-edge-ai|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
