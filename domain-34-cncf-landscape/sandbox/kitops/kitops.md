# KitOps

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kitops.ml/ |
| **GitHub** | https://github.com/jozu-ai/kitops |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KitOps 是一个 MLOps/AI 工件打包和版本管理工具，使用 OCI 标准将 AI/ML 项目的所有组件（模型权重、数据集、代码、配置）打包为称为 ModelKit 的 OCI Artifact。它允许数据科学家和 ML 工程师像管理容器镜像一样管理 AI 模型全生命周期的工件，并通过标准容器注册中心进行分发。

### 核心特性

- **ModelKit**: 将模型、数据集、代码、配置打包为单一 OCI Artifact
- **OCI 标准**: 使用容器注册中心（Docker Hub、GHCR、ECR 等）存储和分发
- **Kitfile**: 声明式 YAML 定义 ModelKit 内容和元数据
- **选择性解包**: 可以只解包需要的部分（如只下载模型不下载数据集）
- **版本管理**: 通过 OCI 标签管理模型版本
- **DevServer**: 内置开发服务器快速验证模型

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│            KitOps 工作流                     │
│                                              │
│  ┌────────────────────────────────┐         │
│  │         Kitfile (YAML)         │         │
│  │  model: model.onnx             │         │
│  │  datasets: training-data/      │         │
│  │  code: src/                    │         │
│  │  config: hyperparams.yaml      │         │
│  └──────────────┬─────────────────┘         │
│                 │ kit pack                   │
│  ┌──────────────▼─────────────────┐         │
│  │     ModelKit (OCI Artifact)    │         │
│  │  ┌──────┐ ┌──────┐ ┌──────┐  │         │
│  │  │Model │ │Data  │ │Code  │  │         │
│  │  │Layer │ │Layer │ │Layer │  │         │
│  │  └──────┘ └──────┘ └──────┘  │         │
│  └──────────────┬─────────────────┘         │
│                 │ kit push                   │
│  ┌──────────────▼─────────────────┐         │
│  │      OCI Registry              │         │
│  │  (Docker Hub / GHCR / ECR)     │         │
│  └──────────────┬─────────────────┘         │
│                 │ kit pull / kit unpack      │
│  ┌──────────────▼─────────────────┐         │
│  │   部署 / 开发 / CI/CD          │         │
│  └────────────────────────────────┘         │
└─────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS
brew install kitops

# Linux
curl -fsSL https://kitops.ml/install.sh | bash

# 验证
kit version
```

### 创建 Kitfile

```yaml
# Kitfile
manifestVersion: "1.0"
package:
  name: sentiment-classifier
  version: "1.0.0"
  description: "Sentiment analysis model based on BERT"
  authors:
    - "ML Team <ml@example.com>"

model:
  name: sentiment-bert
  path: ./models/sentiment_model.onnx
  framework: onnx
  description: "Fine-tuned BERT for sentiment analysis"
  license: Apache-2.0
  parameters:
    hidden_size: 768
    num_labels: 3

datasets:
  - name: training-data
    path: ./data/train.csv
    description: "Training dataset (50k samples)"
  - name: validation-data
    path: ./data/val.csv
    description: "Validation dataset (5k samples)"

code:
  - path: ./src/
    description: "Training and inference code"

docs:
  - path: ./docs/
    description: "Model documentation and evaluation reports"
```

### 打包和推送

```bash
# 打包 ModelKit
kit pack . -t ghcr.io/myorg/sentiment-model:v1.0

# 推送到注册中心
kit push ghcr.io/myorg/sentiment-model:v1.0

# 查看 ModelKit 信息
kit info ghcr.io/myorg/sentiment-model:v1.0
```

### 拉取和解包

```bash
# 拉取 ModelKit
kit pull ghcr.io/myorg/sentiment-model:v1.0

# 完整解包
kit unpack ghcr.io/myorg/sentiment-model:v1.0 -d ./workspace

# 只解包模型文件
kit unpack ghcr.io/myorg/sentiment-model:v1.0 \
  -d ./deploy \
  --model

# 只解包数据集
kit unpack ghcr.io/myorg/sentiment-model:v1.0 \
  -d ./data \
  --datasets
```

### 开发服务器

```bash
# 启动本地推理开发服务器
kit dev start ghcr.io/myorg/sentiment-model:v1.0

# 测试推理
curl http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"messages": [{"role": "user", "content": "This movie was great!"}]}'
```

---

## CI/CD 集成

### GitHub Actions

```yaml
name: Pack and Push ModelKit
on:
  push:
    tags: ['v*']

jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          lfs: true

      - name: Install KitOps
        run: curl -fsSL https://kitops.ml/install.sh | bash

      - name: Login to GHCR
        run: kit login ghcr.io -u ${{ github.actor }} -p ${{ secrets.GITHUB_TOKEN }}

      - name: Pack and Push
        run: |
          kit pack . -t ghcr.io/${{ github.repository }}:${{ github.ref_name }}
          kit push ghcr.io/${{ github.repository }}:${{ github.ref_name }}
```

---

## 与其他方案对比

| 特性 | KitOps | MLflow | DVC | Hugging Face Hub |
|:---|:---|:---|:---|:---|
| 打包格式 | OCI Artifact | MLflow Model | Git + 远程存储 | 专有 |
| 存储后端 | 任意 OCI Registry | S3/Azure/GCS | S3/GCS/Azure | HF Hub |
| 版本管理 | OCI 标签 | 实验追踪 | Git + DVC | Git LFS |
| 选择性下载 | 支持 | 部分 | 支持 | 部分 |
| 标准协议 | OCI 标准 | 专有 API | Git | 专有 API |
| K8s 集成 | OCI 原生 | 需适配 | 需适配 | 需适配 |

---

## 最佳实践

1. **版本策略**: 使用语义版本标签管理 ModelKit，保持可追溯性
2. **分层打包**: 将大文件（模型权重）放在独立层，利用 OCI 层缓存加速拉取
3. **元数据完善**: 在 Kitfile 中详细记录模型参数、训练配置和评估指标
4. **CI/CD 集成**: 训练完成后自动打包推送 ModelKit
5. **安全扫描**: 对 ModelKit 中的代码部分进行安全扫描

---

## 参考资源

- [KitOps 官方文档](https://kitops.ml/docs/)
- [KitOps GitHub](https://github.com/jozu-ai/kitops)
- [Kitfile 规范](https://kitops.ml/docs/kitfile/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
