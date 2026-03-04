# ModelPack

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://modelpack.io/ |
| **GitHub** | https://github.com/modelpack/modelpack |
| **许可证** | Apache-2.0 |
| **开发语言** | Python, Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

ModelPack 是一个 ML/AI 模型打包和分发标准，将机器学习模型、依赖、配置和元数据打包为 OCI 兼容的制品 (Artifact)。它定义了一套标准化的模型打包格式，使模型可以像容器镜像一样在 OCI Registry（如 Docker Hub、Harbor）中存储、版本化和分发，简化从训练到部署的 MLOps 流程。

### 核心特性

- **OCI 兼容**: 模型打包为 OCI Artifact，可存储在任意 OCI Registry
- **标准格式**: 统一的模型打包规范，包含模型文件、配置、依赖和元数据
- **版本管理**: 模型版本通过 OCI tag 管理，支持语义化版本
- **签名验证**: 支持 Sigstore/Cosign 签名，保障模型供应链安全
- **多框架支持**: 支持 PyTorch、TensorFlow、ONNX、HuggingFace 等主流框架
- **元数据标准**: 包含模型卡 (Model Card)、性能指标、许可证等元数据

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  ModelPack Workflow                   │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │             Training Environment               │    │
│  │  ┌──────────────┐  ┌────────────────────────┐│    │
│  │  │ Model Files  │  │ Model Metadata         ││    │
│  │  │ (weights,    │  │ (card, metrics,        ││    │
│  │  │  config)     │  │  license)              ││    │
│  │  └──────┬───────┘  └────────────┬───────────┘│    │
│  └─────────┼───────────────────────┼────────────┘    │
│            │                       │                  │
│  ┌─────────▼───────────────────────▼────────────┐    │
│  │              ModelPack CLI                     │    │
│  │                                                │    │
│  │  modelpack pack \                              │    │
│  │    --model ./model \                           │    │
│  │    --config modelpack.yaml \                   │    │
│  │    --output myrepo/mymodel:v1.0                │    │
│  │                                                │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │         OCI Artifact (ModelPack Format)        │    │
│  │  ┌───────────────────────────────────────┐   │    │
│  │  │ Layer 1: Model Weights (.pt/.onnx)    │   │    │
│  │  │ Layer 2: Config (config.json)         │   │    │
│  │  │ Layer 3: Tokenizer (if applicable)    │   │    │
│  │  │ Layer 4: Model Card (README.md)       │   │    │
│  │  │ Manifest: metadata + signatures       │   │    │
│  │  └───────────────────────────────────────┘   │    │
│  └─────────────────────┬────────────────────────┘    │
└────────────────────────┼─────────────────────────────┘
                         │ Push
                         ▼
              ┌──────────────────┐
              │  OCI Registry     │
              │  (Harbor/Docker   │
              │   Hub/ECR/GCR)   │
              └────────┬─────────┘
                       │ Pull
         ┌─────────────┼─────────────┐
         │             │             │
   ┌─────▼─────┐ ┌─────▼─────┐ ┌────▼──────┐
   │ Inference │ │ Training  │ │ Edge      │
   │ Server    │ │ Pipeline  │ │ Device    │
   └───────────┘ └───────────┘ └───────────┘
```

---

## 快速开始

### 安装 ModelPack CLI

```bash
# 使用 pip 安装
pip install modelpack

# 或使用 brew
brew install modelpack/tap/modelpack

# 验证安装
modelpack version
```

### 创建 ModelPack 配置

```yaml
# modelpack.yaml
apiVersion: modelpack.io/v1
kind: ModelPack
metadata:
  name: text-classifier
  version: 1.0.0
  description: "BERT-based text classification model"
  license: Apache-2.0
  authors:
    - name: ML Team
      email: ml@example.com

spec:
  framework: pytorch
  
  # 模型文件
  model:
    path: ./model
    format: pytorch
    files:
      - pytorch_model.bin
      - config.json
      - vocab.txt
  
  # 运行时依赖
  runtime:
    python: ">=3.9"
    dependencies:
      - torch>=2.0.0
      - transformers>=4.30.0
  
  # 模型卡
  modelCard:
    task: text-classification
    datasets:
      - imdb
    metrics:
      - name: accuracy
        value: 0.92
      - name: f1
        value: 0.91
    intendedUse: "Sentiment analysis for product reviews"
    limitations: "English only, max 512 tokens"
```

### 打包模型

```bash
# 打包模型
modelpack pack \
  --config modelpack.yaml \
  --output myregistry.io/ml/text-classifier:v1.0.0

# 签名模型 (使用 Cosign)
modelpack sign myregistry.io/ml/text-classifier:v1.0.0
```

### 推送到 Registry

```bash
# 登录 Registry
modelpack login myregistry.io

# 推送模型
modelpack push myregistry.io/ml/text-classifier:v1.0.0

# 查看模型信息
modelpack inspect myregistry.io/ml/text-classifier:v1.0.0
```

### 拉取和使用模型

```bash
# 拉取模型
modelpack pull myregistry.io/ml/text-classifier:v1.0.0 -o ./model

# 验证签名
modelpack verify myregistry.io/ml/text-classifier:v1.0.0
```

```python
# 在代码中加载
from modelpack import load_model

model = load_model("myregistry.io/ml/text-classifier:v1.0.0")
result = model.predict("This product is amazing!")
```

---

## 高级功能

### 多框架支持

```yaml
# PyTorch 模型
spec:
  framework: pytorch
  model:
    format: pytorch
    files:
      - pytorch_model.bin
      - config.json

# ONNX 模型
spec:
  framework: onnx
  model:
    format: onnx
    files:
      - model.onnx

# HuggingFace 模型
spec:
  framework: huggingface
  model:
    format: transformers
    files:
      - pytorch_model.bin
      - config.json
      - tokenizer.json
      - vocab.txt
```

### Kubernetes 集成

```yaml
# 在 K8s 中使用 ModelPack
apiVersion: serving.kubeflow.org/v1beta1
kind: InferenceService
metadata:
  name: text-classifier
spec:
  predictor:
    model:
      modelFormat:
        name: modelpack
      storageUri: "oci://myregistry.io/ml/text-classifier:v1.0.0"
```

### 模型版本对比

```bash
# 对比两个版本的模型
modelpack diff \
  myregistry.io/ml/text-classifier:v1.0.0 \
  myregistry.io/ml/text-classifier:v2.0.0

# 输出:
# Files changed:
# + pytorch_model.bin (size: 438MB -> 512MB)
# ~ config.json (hidden_size: 768 -> 1024)
# Metrics changed:
# accuracy: 0.92 -> 0.94
```

### 供应链安全

```bash
# 签名模型
modelpack sign \
  --key cosign.key \
  myregistry.io/ml/text-classifier:v1.0.0

# 附加 SBOM
modelpack sbom attach \
  --sbom sbom.spdx.json \
  myregistry.io/ml/text-classifier:v1.0.0

# 验证签名和 SBOM
modelpack verify \
  --key cosign.pub \
  --sbom-required \
  myregistry.io/ml/text-classifier:v1.0.0
```

---

## 与其他方案对比

| 特性 | ModelPack | MLflow | KitOps | HuggingFace Hub |
|:---|:---|:---|:---|:---|
| 打包格式 | OCI Artifact | MLflow Format | OCI + ModelKit | Git LFS |
| Registry | 任意 OCI Registry | MLflow Server | 任意 OCI Registry | HF Hub |
| 签名 | Cosign 原生 | 无 | 有限 | 无 |
| K8s 集成 | 原生 | 需适配 | 原生 | 需适配 |
| 元数据 | Model Card 标准 | MLmodel | 内置 | Model Card |
| 多框架 | PyTorch/TF/ONNX | 多框架 | 多框架 | 主要 HF |

---

## 最佳实践

1. **语义版本**: 使用语义化版本号管理模型版本（major.minor.patch）
2. **Model Card**: 填写完整的 Model Card，包括用途、限制、偏见说明
3. **签名验证**: 生产环境始终验证模型签名，防止模型被篡改
4. **依赖锁定**: 精确指定 Python 和库版本，保证可复现性
5. **CI/CD 集成**: 将 modelpack pack/push 集成到 ML Pipeline

---

## 参考资源

- [ModelPack 官方文档](https://modelpack.io/docs/)
- [ModelPack GitHub](https://github.com/modelpack/modelpack)
- [OCI Distribution Spec](https://github.com/opencontainers/distribution-spec)
- [Model Card 标准](https://modelcards.withgoogle.com/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
