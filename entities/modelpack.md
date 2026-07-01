---
title: ModelPack (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- image
- modelpack
- docker
- harbor
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ModelPack 是什么
- 如何 ModelPack
trigger_keywords:
- ModelPack
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# ModelPack

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Python, Go

## 概述

ModelPack 是一个 ML/AI 模型打包和分发标准，将机器学习模型、依赖、配置和元数据打包为 OCI 兼容的制品 (Artifact)。它定义了一套标准化的模型打包格式，使模型可以像容器镜像一样在 OCI Registry（如 Docker Hub、Harbor）中存储、版本化和分发，简化从训练到部署的 MLOps 流程。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **语义版本**: 使用语义化版本号管理模型版本（major.minor.patch）
- **Model Card**: 填写完整的 Model Card，包括用途、限制、偏见说明
- **签名验证**: 生产环境始终验证模型签名，防止模型被篡改
- **依赖锁定**: 精确指定 Python 和库版本，保证可复现性
- **CI/CD 集成**: 将 modelpack pack/push 集成到 ML Pipeline

## 架构定位

在 CNCF 生态中，modelpack 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/storage-model.md|storage-model]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[drasi]] — Drasi
- [[containerssh]] — ContainerSSH
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[docker]] — Docker
- [[harbor]] — Harbor

- modelpack
- [[entities/eraser.md|[[Eraser|Eraser]]]]
- [[entities/slimtoolkit.md|[[SlimToolkit|SlimToolkit]]]]
- [[entities/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
