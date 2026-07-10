---
title: 大语言模型（LLM）推理优化
description: '# 大语言模型（LLM）推理优化'
summary: '# 大语言模型（LLM）推理优化'
category: dictionary
tags:
- k8s
- glossary
- terminology
- gpu
- nvidia
- vllm
- llm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 大语言模型（LLM）推理优化 是什么
- 如何 大语言模型（LLM）推理优化
trigger_keywords:
- 大语言模型
- LLM
- 推理优化
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 大语言模型（LLM）推理优化

## 概述

随着大语言模型（LLM）在生产环境的广泛部署，推理成本已成为 AI 基础设施的最大开支项。2026 年的最佳实践表明，通过 **Continuous Batching（连续批处理）、Quantization（量化）、Parallelism（并行策略）** 以及 **Prefill/Decode 分离** 等优化手段，可以将 LLM 推理的 GPU 利用率从约 40% 提升至 90% 以上，并将单 token 成本降低 85%。

## 核心概念/原理

### 1. Continuous Batching（连续批处理）

传统的静态批处理（Static Batching）要求一个批次内的所有请求同时开始、同时结束。由于 LLM 生成长度差异大，这会导致严重的**尾部延迟问题**和 GPU 空闲。

**Continuous Batching**（也称为 In-Flight Batching 或 Iteration-level Batching）允许：
- 在每次模型前向传播迭代时，动态地将新请求加入当前批次
- 已完成的请求可以随时退出批次
- 显著提升 GPU 利用率（从 ~40% 提升至 ~90%）

> 研究表明，batch size 从 1 提升到 32 可将每 token 成本降低约 85%，而延迟仅增加约 20%。

### 2. 量化（Quantization）

量化通过降低模型权重精度来减少显存占用和提升推理速度：
- **FP16 → INT8**：速度和显存改善约 2 倍
- **INT8 → INT4/GPTQ/AWQ**：模型大小可缩减 50%–75%，部分场景下精度损失可接受
- **8-bit 量化实例**：Mercari 通过 8-bit 量化将 GPT 级别模型大小减少 95%，推理成本降低 14 倍

### 3. Prefill 与 Decode 分离

LLM 推理包含两个计算特征截然不同的阶段：
- **Prefill Phase（预填充）**：处理输入 prompt，计算密集（Compute-bound）
- **Decode Phase（解码生成）**：逐 token 生成，内存带宽受限（Memory-bound）

**分离优化策略**：
- 为 Prefill 和 Decode 分配不同批大小和硬件配置
- Prefill 使用计算更强的 GPU，Decode 使用高内存带宽 GPU
- 部分先进系统支持在同一 GPU 上流水线重叠 Prefill 和 Decode

### 4. 并行策略

| 并行方式 | 原理 | 适用场景 |
|----------|------|----------|
| **Data Parallelism** | 复制完整模型到多个 GPU | 中小模型、高并发请求 |
| **Tensor Parallelism** | 将单层计算拆分到多个 GPU | 单 GPU 放不下的大模型 |
| **Pipeline Parallelism** | 将模型不同层拆分到多个 GPU | 超大规模模型跨节点部署 |

研究表明，合理的并行配置匹配可将系统性能提升高达 **2.61 倍**，成本效率提升 **2.27 倍**。

## 关键机制或特性

### vLLM 与 PagedAttention

**vLLM** 是 2025–2026 年最流行的开源 LLM 推理引擎之一，其核心创新 **PagedAttention** 将 KV Cache 按块（block）管理，类似操作系统的虚拟内存分页：
- 消除显存碎片
- 支持更大 batch size
- 显著提升吞吐量

### 多级缓存系统

- **Prefix Caching**：缓存相同 prompt 前缀的 KV Cache，避免重复计算
- **Request Deduplication**：对完全相同的请求返回缓存结果
- **Drobox 案例**：通过多级缓存大幅减少 LLM 调用次数

### 请求聚合与分形调度

- **Request Aggregation**：在网关层将短时间窗口内的相似请求合并为批次
- **Priority Scheduling**：在资源争用时保护高优先级交互式请求

## 使用场景

1. **高并发聊天机器人**：使用 vLLM + Continuous Batching 处理数千并发对话请求
2. **低延迟 API 服务**：对交互式应用采用小 batch size + INT8 量化，平衡延迟与成本
3. **批量文档处理**：对非实时任务采用大 batch size + 可抢占 Spot GPU，最大化吞吐
4. **超大模型部署（100B+ 参数）**：采用 Tensor + Pipeline 并行跨多机多卡部署

## 最佳实践/注意事项

- **监控 per-token 成本与延迟**：优化决策必须同时考虑这两个指标，不可只追求吞吐
- **按延迟等级设置 batch 上限**：交互式任务 batch 上限较低，批处理任务可适当放宽
- **量化前进行精度评估**：INT4 量化虽然显存收益巨大，但需在生产流量上验证模型输出质量
- **合理限制 prompt 长度**：更短的 prompt 意味着更快的 Prefill 和更少显存占用
- **优先使用支持 Continuous Batching 的运行时**：如 vLLM、TensorRT-LLM、Triton + Inflight Batcher
- **模型蒸馏替代直接部署超大模型**：LinkedIn 案例显示，通过模型蒸馏在保持相近准确率的同时大幅降低成本和延迟

## 参考链接

- [vLLM Documentation](https://docs.vllm.ai/)
- [NVIDIA TensorRT-LLM](https://developer.nvidia.com/tensorrt-llm)
- [Mirantis - LLM Optimization Techniques](https://www.mirantis.com/blog/llm-optimization-techniques/)
- [Gun.io - Scaling AI Infrastructure for LLMs](https://gun.io/news/2025/04/scaling-ai-infrastructure-for-llms/)

## Related

- [[系统基础/topic-dictionary/workloads/pod.md|Pod]]
- [[系统基础/topic-dictionary/fundamentals/container.md|Container]]
- [[系统基础/topic-dictionary/fundamentals/node.md|Node]]
- [[系统基础/topic-dictionary/fundamentals/namespace.md|Namespace]]
- [[系统基础/topic-dictionary/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
