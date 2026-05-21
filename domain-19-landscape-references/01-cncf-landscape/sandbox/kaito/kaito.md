---
title: KAITO (Kubernetes AI Toolchain Operator)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- falco
- crd
- operator
- gpu
- nvidia
- llm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KAITO (Kubernetes AI Toolchain Operator) 是什么
- 如何 KAITO (Kubernetes AI Toolchain Operator)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KAITO
- Kubernetes
- AI
- Toolchain
- Operator
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gpu-scheduling-basics
---

title: KAITO (Kubernetes AI Toolchain Operator)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- falco
- crd
- operator
- gpu
- nvidia
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KAITO (Kubernetes AI Toolchain Operator) 是什么
- 如何 KAITO (Kubernetes AI Toolchain Operator)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KAITO
- Kubernetes
- AI
- Toolchain
- Operator
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# KAITO (Kubernetes AI Toolchain Operator)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kaito.sh/ |
| **GitHub** | https://github.com/kaito-project/kaito |
| **许可证** | MIT |
| **开发语言** | Go, Python |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KAITO 是一个 Kubernetes Operator，简化在 Kubernetes 集群上运行 AI/ML 推理和微调工作负载的流程。它自动化了 GPU 节点的配置、模型下载和推理服务部署，使开发者只需指定模型名称即可部署 AI 推理服务。

### 核心特性

- **自动 GPU 配置**: 自动为 AI 工作负载配置合适的 GPU 节点
- **预置模型**: 内置 Llama, Falcon, Mistral, Phi 等流行 LLM 的优化配置
- **一键部署**: 声明式 CRD 部署 AI 推理服务
- **模型微调**: 支持 LoRA 和 QLoRA 微调
- **多 GPU 支持**: 自动处理模型并行和多 GPU 分片
- **云集成**: 与 Azure Karpenter 集成实现 GPU 节点自动伸缩

---

## 快速开始

### 安装

```bash
helm repo add kaito https://kaito-project.github.io/kaito/charts
helm install kaito-workspace kaito/kaito-workspace \
  --namespace kaito-workspace \
  --create-namespace
```

### 部署 LLM 推理服务

```yaml
apiVersion: kaito.sh/v1alpha1
kind: Workspace
metadata:
  name: llama-2-7b
spec:
  resource:
    instanceType: "Standard_NC24ads_A100_v4"  # GPU 节点类型
    labelSelector:
      matchLabels:
        apps: llama-2
    count: 1  # GPU 节点数量
  inference:
    preset:
      name: "llama-2-7b-chat"
    # 或自定义模型
    # template:
    #   containers:
    #     - name: inference
    #       image: my-model:latest
    #       resources:
    #         limits:
    #           nvidia.com/gpu: "1"
```

### 调用推理 API

```bash
# 获取推理服务端点
kubectl get workspace llama-2-7b -o jsonpath='{.status.workerNodes}'

# 调用推理 API (OpenAI 兼容)
curl http://llama-2-7b-service:80/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama-2-7b-chat",
    "messages": [{"role": "user", "content": "What is Kubernetes?"}],
    "temperature": 0.7,
    "max_tokens": 500
  }'
```

### 模型微调

```yaml
apiVersion: kaito.sh/v1alpha1
kind: Workspace
metadata:
  name: llama-2-finetune
spec:
  resource:
    instanceType: "Standard_NC24ads_A100_v4"
    count: 1
  tuning:
    preset:
      name: "llama-2-7b"
    method: qlora
    input:
      urls:
        - "https://storage.example.com/training-data.jsonl"
    output:
      image: "registry.example.com/my-finetuned-model:v1"
      imagePushSecret: registry-secret
    config:
      TrainingArguments:
        num_train_epochs: 3
        per_device_train_batch_size: 4
        learning_rate: 0.0002
      LoraConfig:
        r: 16
        lora_alpha: 32
```

---

## 支持的模型

| 模型 | 参数规模 | GPU 需求 |
|:---|:---|:---|
| Llama 2 7B | 7B | 1x A100 |
| Llama 2 13B | 13B | 2x A100 |
| Llama 2 70B | 70B | 8x A100 |
| Falcon 7B | 7B | 1x A100 |
| Falcon 40B | 40B | 4x A100 |
| Mistral 7B | 7B | 1x A100 |
| Phi-2 | 2.7B | 1x T4/A10 |

---

## 最佳实践

1. **模型选择**: 根据延迟和资源预算选择合适的模型规模
2. **GPU 类型**: 推理用 A10/T4 即可，训练/微调推荐 A100
3. **量化**: 使用 GPTQ/AWQ 量化模型减少 GPU 内存需求
4. **自动伸缩**: 配合 Karpenter 实现 GPU 节点的自动扩缩容
5. **微调数据**: 使用高质量的领域数据进行 LoRA 微调

---

## 参考资源

- [KAITO 官方文档](https://kaito.sh/docs/)
- [KAITO GitHub](https://github.com/kaito-project/kaito)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
