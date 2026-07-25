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

ModelPack 通过 KServe、Seldon Core 或 KubeFlow 等 Kubernetes 原生推理框架实现部署集成。模型制品存储在 OCI Registry 中（如 Harbor），推理 Pod 启动时通过 init-container 拉取模型。通过 CRD（如 KServe InferenceService）定义模型引用，控制器自动解析 ModelPack 制品并配置推理环境。结合 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Secret 管理 Registry 凭证，通过 Cosign 策略控制器验证模型签名。

## 生产场景

1. **企业模型仓库**: 统一管理所有团队训练的模型，通过 Harbor + ModelPack 构建模型仓库
2. **CI/CD 模型部署**: 在训练 Pipeline 中自动打包、签名和推送模型，触发推理服务更新
3. **多环境模型部署**: 同一模型制品在不同环境（dev/staging/prod）间传递，保证一致性
4. **合规审计**: 通过 Model Card 和签名链追溯模型来源、训练数据和版本历史

## 安装与配置

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

### modelpack.yaml 配置示例

```yaml
# modelpack.yaml - 模型清单文件
apiVersion: modelpack.io/v1
kind: ModelPack
metadata:
  name: llama-7b-chat
  version: "1.0.0"
  labels:
    team: ai-platform
    stage: production
spec:
  model:
    format: pytorch
    path: ./weights/model.pt
    framework: pytorch
    framework_version: "2.1.0"
  runtime:
    engine: vllm
    python_version: "3.11"
    cuda_version: "12.1"
    dependencies:
      - vllm==0.3.0
      - transformers==4.36.0
  model_card:
    description: "7B 参数对话模型"
    intended_use: "客服对话、知识问答"
    limitations: "不支持多模态输入"
    training_data: "内部客服语料 + 开源数据集"
  signature:
    provider: sigstore
    key_ref: cosign.key
```

## 运维操作

```bash
# 🟢 查看模型列表
modelpack list --registry=harbor.example.com

# 🟢 查看模型元数据
modelpack inspect myorg/llama-7b:1.0.0

# 🟡 签名模型
modelpack sign myorg/llama-7b:1.0.0 --key=cosign.key

# 🟢 验证签名
modelpack verify myorg/llama-7b:1.0.0

# 🟡 拉取模型到本地
modelpack pull myorg/llama-7b:1.0.0 -o ./models/

# 🟢 查看模型历史版本
modelpack history myorg/llama-7b

# 🔴 删除 Registry 中的模型
modelpack delete myorg/llama-7b:0.9.0 --force
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 模型拉取失败 | Registry 认证问题 | `modelpack pull --debug` | 配置 Registry 凭证 |
| 签名验证失败 | 密钥不匹配 | `modelpack verify --verbose` | 检查密钥对和签名链 |
| 推理失败 | 依赖版本不兼容 | 检查 runtime 配置 | 确认 CUDA/Python 版本匹配 |
| 打包过大 | 未优化模型文件 | `modelpack inspect --layers` | 使用量化/剪枝减小模型 |
| KServe 加载失败 | OCI 格式不兼容 | 检查 InferenceService 日志 | 确认 media-type 正确 |

## 生产案例

### 案例1: 企业模型仓库建设

**场景**: 5个 AI 团队各自管理模型，版本混乱、无法追溯  
**方案**: Harbor + ModelPack 统一模型仓库，强制 Model Card 和签名  
**效果**: 模型可追溯、可复现，部署时间从 2小时缩短到 10分钟  

### 案例2: 模型供应链安全

**场景**: 需确保生产模型未被篡改，满足合规审计要求  
**方案**: Sigstore 签名 + Cosign 策略控制器 + 准入 Webhook 验证  
**效果**: 未签名模型无法部署到生产集群  

## 对比

| 特性 | ModelPack | KitOps | ONNX | Hugging Face Hub |
|------|-----------|--------|------|------------------|
| 打包格式 | OCI Artifact | OCI Artifact | 自定义二进制 | Git LFS |
| Model Card | ✅ 标准化 | ✅ | ❌ | ✅ |
| 供应链安全 | ✅ Sigstore | ✅ Cosign | ❌ | ⚠️ |
| K8s 集成 | ✅ KServe | ✅ | ⚠️ 间接 | ⚠️ |
| 私有化部署 | ✅ 任何 OCI Registry | ✅ | N/A | ⚠️ 需配置 |

## 架构定位

在 CNCF 生态中，ModelPack 属于 **Image** 类别，为云原生 AI/ML 应用提供模型标准化打包和分发能力。

## 检查清单

- [ ] 所有生产模型配置 Model Card
- [ ] 模型制品使用 Sigstore 签名
- [ ] 配置准入策略拒绝未签名模型
- [ ] 模型依赖版本精确锁定
- [ ] 配置 Registry 存储配额和清理策略
- [ ] 建立模型版本发布审批流程

## 参考链接

- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]
- [[23-实体/08-交付与制品/harbor.md|Harbor]]

## Related

- [[drasi]] — Drasi
- [[containerssh]] — ContainerSSH
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[docker]] — Docker
- [[harbor]] — Harbor
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]


<!-- risk-assessed -->
