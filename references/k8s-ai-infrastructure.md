---
title: AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化
description: '## GPU 调度'
category: reference
tags:
- k8s
- ai-infra
- gpu
- distributed-training
- llm
- cost-optimization
- job
- operator
- nvidia
- kubeflow
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化 是什么
- 如何 AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化
trigger_keywords:
- AI
- 基础设施：GPU
- 调度
- 分布式训练
- LLM
- 推理与成本优化
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

# AI 基础设施

## GPU 调度

K8s GPU 资源管理方式：
- **Device Plugin**：暴露 `nvidia.com/gpu` 资源
- **MIG（Multi-Instance GPU）**：A100/H100 拆分多实例
- **MPS（Multi-Process Service）**：GPU 共享
- **GPU Operator**：自动化 GPU 驱动安装与管理

## 分布式训练

| 框架 | 特点 | 适用场景 |
|------|------|----------|
| PyTorch DDP | 数据并行 | 中小模型 |
| DeepSpeed | ZeRO 优化 | 大模型训练 |
| Megatron-LM | 模型并行/Pipeline 并行 | 超大模型 |
| KubeFlow Training | K8s 原生 | 通用 |

关键 K8s 资源：PyTorchJob、TFJob、MPIJob（KubeFlow Operator）。

## LLM 推理优化

- **vLLM**：PagedAttention，连续批处理，显存高效
- **TensorRT-LLM**：NVIDIA 优化推理引擎
- **Triton Inference Server**：多模型服务
- **量化**：GPTQ/AWQ/GGUF 降低显存需求

## 成本优化

- Spot/抢占式实例 + 检查点恢复
- 资源请求精确化（避免过度请求）
- 模型蒸馏/量化减少计算需求
- 推理自动缩容（基于请求量）

---

> 来源：.zread/wiki/drafts/17-ai-ji-chu-she-shi-*.md

## Related

- [[references/k8s-ai-infra-domain-guide.md|k8s-ai-infra-domain-guide]] — AI Infrastructure on Kubernetes Domain Guide
- [[kubeflow]] — Kubeflow
