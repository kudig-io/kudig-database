---
title: KServe 模型服务平台
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- istio
- hpa
- job
- networkpolicy
- crd
- gpu
- nvidia
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KServe 模型服务平台 是什么
- 如何 KServe 模型服务平台
trigger_keywords:
- KServe
- 模型服务平台
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- service-mesh-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[KServe|KServe]] 模型服务平台

## 概述

**KServe** 是 [[Kubernetes|Kubernetes]] 上领先的**云原生模型推理服务平台**，提供标准化的模型部署、自动扩缩容（包括缩至零）、金丝雀发布、A/B 测试以及多框架支持。作为 CNCF 孵化的项目，KServe 在 2025–2026 年已成为企业级 AI 推理基础设施的事实标准。

## 核心概念/原理

### 1. 标准化推理服务抽象

KServe 将模型服务抽象为 `InferenceService` 自定义资源（CRD），屏蔽了底层框架差异。用户只需声明模型存储位置（如 S3、GCS、PVC）和框架类型（如 TensorFlow、PyTorch、SKLearn、XGBoost、HuggingFace），KServe 会自动生成对应的推理运行时容器。

### 2. 自动扩缩容与 Scale-to-Zero

KServe 深度集成 **[[knative|[[Knative]]]]** 和 **[[Istio|Istio]]**，支持：
- **HPA 基于自定义指标扩缩容**：如 GPU 利用率、请求队列长度、推理延迟
- **Scale-to-Zero**：当请求量为零时自动缩容至 0 Pod，显著降低空闲 GPU/CPU 成本
- **冷启动优化**：通过模型预加载、镜像缓存、容器启动加速减少从 0 到 1 的延迟

### 3. 多框架运行时（Runtimes）

KServe 提供一系列预构建的 Serving Runtime：
- **TensorFlow Serving**：高性能 TF 模型服务
- **TorchServe**：PyTorch 模型服务
- **Triton Inference Server**：NVIDIA 的多框架推理引擎，支持 GPU 加速
- **vLLM**：专门针对大语言模型（LLM）的高吞吐量推理运行时，支持 Continuous Batching
- **HuggingFace Transformers**：便捷的 HuggingFace 模型部署

### 4. 高级流量管理

- **金丝雀发布（Canary Rollout）**：逐步将流量从旧模型版本切换到新版本
- **A/B 测试**：将流量按比例分配到不同模型版本进行对比评估
- **模型解释器（Explainer）**：集成 AI 可解释性工具（如 Alibi、LIME）
- **模型转换器（Transformer）**：在请求到达模型前进行预处理或在响应后进行后处理

## 关键机制或特性

### InferenceService 生命周期

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-inference
spec:
  predictor:
    model:
      modelFormat:
        name: huggingface
      storageUri: s3://models/llm-v1
      resources:
        limits:
          nvidia.com/gpu: 1
```

- `predictor`：必需的模型推理端点
- `transformer`：可选的请求/响应转换逻辑
- `explainer`：可选的模型解释端点

### 多协议支持

KServe 支持多种推理协议：
- **REST/HTTP**：通用、易于集成
- **gRPC**：高性能、低延迟
- **Open Inference Protocol（OIP）**：跨运行时标准化的推理请求/响应格式

## 使用场景

1. **LLM 推理服务部署**：使用 vLLM Runtime 部署大语言模型，实现高吞吐量和连续批处理
2. **CV/NLP 模型微服务化**：将训练好的 TensorFlow/PyTorch 模型快速发布为可弹性伸缩的 API 服务
3. **模型版本管理与灰度发布**：通过金丝雀和 A/B 测试验证新模型版本的准确性和延迟表现
4. **成本敏感型推理**：利用 Scale-to-Zero 功能，在低峰期自动释放 GPU 资源，按需付费

## 最佳实践/注意事项

- **选择合适的 Runtime**：LLM 场景优先选择 vLLM 或 Triton；通用 CV/NLP 可选 TorchServe
- **配置合理的扩缩容指标**：不要仅依赖 CPU，应结合 GPU 利用率、请求队列深度或推理延迟
- **模型存储优化**：将大模型存储在集群本地的高速存储（如 NVMe PVC）或对象存储近端缓存，减少加载时间
- **安全隔离**：为不同业务线的 InferenceService 配置独立的 Namespace、ResourceQuota 和 NetworkPolicy
- **监控推理 SLA**：重点监控 P50/P95/P99 延迟、吞吐量（tokens/sec）、错误率和 GPU 利用率
- **预热与缓存**：对于不能容忍冷启动的场景，设置最小副本数为 1，或配合模型预热 Job

## 参考链接

- [KServe Official Documentation](https://kserve.github.io/website/latest/)
- [KServe GitHub Repository](https://github.com/kserve/kserve)
- [vLLM Documentation](https://docs.vllm.ai/)
- [NVIDIA Triton Inference Server](https://developer.nvidia.com/triton-inference-server)

## Related
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
