---
title: ModelPack (entities)
description: '## 概述'
summary: 'ModelPack 是一个 ML/AI 模型打包和分发标准，将机器学习模型、依赖、配置和元数据打包为 OCI 兼容的制品 (Artifact)。'
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
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# ModelPack

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Python, Go

## 概述

ModelPack 是一个 ML/AI 模型打包和分发标准，由云原生社区推动，2024 年加入 CNCF 沙箱。它将机器学习模型、依赖、配置和元数据打包为 OCI 兼容的制品（Artifact），使模型可以像容器镜像一样在 OCI Registry（如 Docker Hub、Harbor）中存储、版本化和分发。ModelPack 定义了一套标准化的模型打包格式，包含 Model Card（模型卡片）、推理配置、依赖清单和签名信息，简化从训练到部署的 MLOps 流程。它支持 PyTorch、TensorFlow、ONNX、Hugging Face 等主流模型格式，并内置供应链安全验证。

## 核心能力

- **OCI 兼容打包**: 将 ML 模型打包为 OCI 制品，复用现有 Registry 基础设施（Harbor、ACR、ECR）
- **Model Card 元数据**: 标准化模型用途、限制、偏见说明、训练数据来源等元信息
- **依赖锁定**: 精确指定 Python 版本、CUDA 版本和依赖库版本，保证可复现性
- **签名验证**: 基于 Sigstore 的模型签名，防止模型被篡改
- **多格式支持**: PyTorch (.pt)、TensorFlow (SavedModel)、ONNX、Hugging Face 等主流格式
- **RAG 文档打包**: 支持将向量索引、嵌入模型和文档集打包为统一制品

## 架构

ModelPack 采用 OCI 制品规范扩展模型打包：

- **modelpack.yaml**: 模型清单文件，定义模型文件、依赖、推理引擎和元数据
- **Model Artifact**: OCI 制品层，包含模型权重（blob）、配置文件（layer）和签名（manifest）
- **modelpack CLI**: 打包和推送工具，类似 `docker build/push` 但针对模型优化
- **Registry 扩展**: 兼容标准 OCI Registry，通过 Manifest media-type 区分模型制品
- **推理运行时**: 部署端解析 ModelPack 制品，自动配置推理环境（如 KServe、Seldon）

打包流程：`modelpack.yaml → modelpack build → OCI Artifact → Registry → modelpack pull → 推理引擎`

## K8s 集成

ModelPack 通过 KServe、Seldon Core 或 KubeFlow 等 Kubernetes 原生推理框架实现部署集成。模型制品存储在 OCI Registry 中（如 Harbor），推理 Pod 启动时通过 init-container 拉取模型。通过 CRD（如 KServe InferenceService）定义模型引用，控制器自动解析 ModelPack 制品并配置推理环境。结合 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Secret 管理 Registry 凭证，通过 Cosign 策略控制器验证模型签名。

## 生产场景

1. **企业模型仓库**: 统一管理所有团队训练的模型，通过 Harbor + ModelPack 构建模型仓库
2. **CI/CD 模型部署**: 在训练 Pipeline 中自动打包、签名和推送模型，触发推理服务更新
3. **多环境模型部署**: 同一模型制品在不同环境（dev/staging/prod）间传递，保证一致性
4. **合规审计**: 通过 Model Card 和签名链追溯模型来源、训练数据和版本历史

## 安装

```bash
# 安装 modelpack CLI
pip install modelpack

# 打包模型
modelpack build ./my-model --name myorg/llama-7b:1.0.0

# 推送到 Registry
modelpack push myorg/llama-7b:1.0.0

# 拉取并验证
modelpack pull myorg/llama-7b:1.0.0 --verify-signature
```

## 对比

| 特性 | ModelPack | KitOps | ONNX |
|------|-----------|--------|------|
| 打包格式 | OCI Artifact | OCI Artifact | 自定义二进制 |
| Model Card | ✅ 标准化 | ✅ | ❌ |
| 供应链安全 | ✅ Sigstore 签名 | ✅ Cosign | ❌ |
| K8s 集成 | ✅ KServe | ✅ | ⚠️ 间接 |

## 架构定位

在 CNCF 生态中，ModelPack 属于 **Image** 类别，为云原生 AI/ML 应用提供模型标准化打包和分发能力。

## 参考链接

- [[概念/storage-model.md|storage-model]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[drasi]] — Drasi
- [[containerssh]] — ContainerSSH
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[docker]] — Docker
- [[harbor]] — Harbor

- modelpack
- [[实体/eraser.md|[[Eraser|Eraser]]]]
- [[实体/slimtoolkit.md|[[SlimToolkit|SlimToolkit]]]]
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
