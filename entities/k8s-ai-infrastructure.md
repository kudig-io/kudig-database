---
title: AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化
description: '## GPU 调度'
summary: '关键 K8s 资源：PyTorchJob、TFJob、MPIJob（KubeFlow Operator）。'
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
tier: supporting
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[entities/k8s-ai-infra-domain-guide.md|k8s-ai-infra-domain-guide]] — AI Infrastructure on Kubernetes Domain Guide
- [[kubeflow]] — Kubeflow


<!-- risk-assessed -->
