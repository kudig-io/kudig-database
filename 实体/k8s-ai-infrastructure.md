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
last_updated: 2026-07
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

## 概述

Kubernetes 已成为 AI/ML 基础设施的标准编排平台。本页概述在 K8s 上运行 AI 工作负载的四大核心领域：**GPU 资源调度与管理**、**分布式训练编排**、**LLM 推理服务部署**和**成本优化**。这些能力依赖于 KubeFlow Training Operator、NVIDIA GPU Operator、Volcano/Kueue 批调度器、KServe 推理框架等 CNCF 和生态系统项目。

## GPU 调度

K8s GPU 资源管理方式：

- **Device Plugin**：NVIDIA Device Plugin 暴露 `nvidia.com/gpu` 资源，kubelet 自动分配
- **MIG（Multi-Instance GPU）**：A100/H100 GPU 物理分区为 7 个独立实例，每个实例隔离运行
- **MPS（Multi-Process Service）**：多个进程共享同一 GPU 的计算资源，提升利用率
- **GPU Operator**：NVIDIA GPU Operator 自动化管理 GPU 驱动安装、容器运行时配置、DCGM 监控
- **vGPU/GPU 共享**：通过时间分片或内存分区实现 GPU 共享（如 HAMI、GPU Manager）

```yaml
# 请求 GPU 的 Pod 示例
spec:
  containers:
  - name: training
    image: pytorch/pytorch:latest
    resources:
      limits:
        nvidia.com/gpu: 1
```

## 分布式训练

| 框架 | 特点 | 适用场景 |
|------|------|----------|
| PyTorch DDP | 数据并行 | 中小模型 |
| DeepSpeed | ZeRO 优化 | 大模型训练 |
| Megatron-LM | 模型并行/Pipeline 并行 | 超大模型 |
| KubeFlow Training | K8s 原生 | 通用 |

关键 K8s 资源：`PyTorchJob`、`TFJob`、`MPIJob`（通过 Kubeflow Training Operator）。这些 CRD 管理分布式训练的 Worker/PodLauncher 生命周期，自动处理节点亲和性、GPU 分配和故障恢复。配合 Kueue 或 Volcano 实现训练任务的队列化调度和公平资源分配。

## LLM 推理优化

- **vLLM**：PagedAttention 技术，连续批处理（Continuous Batching），显存利用率高，吞吐量大
- **TensorRT-LLM**：NVIDIA 优化推理引擎，支持 INT8/FP8 量化，延迟最优
- **Triton Inference Server**：多模型多框架服务，支持动态 batching
- **量化**：GPTQ/AWQ/GGUF 降低显存需求（FP16→INT4 可减少 75% 显存）
- **推理框架**：KServe（标准推理服务）、Text Generation Inference (TGI)

## 成本优化

- **Spot/抢占式实例 + 检查点恢复**：训练任务使用 Spot 实例降低 70% 成本，配合定期 Checkpoint 实现容错
- **资源请求精确化**：根据实际利用率调整 GPU 请求量，避免过度请求
- **模型蒸馏/量化**：减少推理计算需求，INT4 量化可使吞吐提升 2-3 倍
- **推理自动缩容**：基于请求量自动扩缩，空闲时缩容到零（配合 KServe）
- **GPU 共享**：多个低 QPS 推理服务共享同一 GPU（通过 MPS 或时间分片）

## 生产部署要点

- **GPU 监控**：通过 DCGM Exporter 暴露 GPU 利用率、显存、温度指标到 Prometheus
- **训练容错**：定期 Checkpoint 到对象存储（S3/OSS），节点故障时从最近 Checkpoint 恢复
- **数据本地化**：使用节点本地存储（如 HwameiStor）缓存训练数据，减少网络 I/O
- **队列管理**：使用 Kueue 管理训练任务队列，避免资源争抢
- **弹性训练**：结合 Volcano 弹性训练能力，节点扩缩容时自动调整 Worker 数量

---

> 来源：.zread/wiki/drafts/17-ai-ji-chu-she-shi-*.md

## Related

- [[实体/k8s-ai-infra-domain-guide.md|k8s-ai-infra-domain-guide]] — AI Infrastructure on Kubernetes Domain Guide
- [[kubeflow]] — Kubeflow


<!-- risk-assessed -->
