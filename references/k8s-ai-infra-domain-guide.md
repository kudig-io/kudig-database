---
title: AI Infrastructure on Kubernetes Domain Guide
description: AI Infrastructure on Kubernetes Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- ai
- ml
- gpu
- llm
- domain-14-ai-ml-infra
- reference
- prometheus
- job
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI Infrastructure on Kubernetes Domain Guide 是什么
- 如何 AI Infrastructure on Kubernetes Domain Guide
trigger_keywords:
- AI
- Infrastructure
- 'on'
- Kubernetes
- Domain
- Guide
prerequisites:
- kubectl-basics
- prometheus-basics
- gpu-scheduling-basics
---

# AI Infrastructure on Kubernetes Domain Guide

## Source

Distilled from domain-11-ai-infra (37 documents, Kubernetes v1.28-v1.32).

## GPU Management

- **Device Plugin**: Exposes GPUs as schedulable resources (`nvidia.com/gpu`)
- **GPU sharing**: Time-slicing (multiple workloads share one GPU) or MIG (Multi-Instance GPU for A100+)
- **Scheduling**: Extended resources with node selectors for GPU type
- **Monitoring**: DCGM (Data Center GPU Manager) for GPU metrics (utilization, memory, temperature, power)

## Distributed Training

- **Frameworks**: PyTorch distributed, TensorFlow, Horovod, Megatron-LM
- **Communication**: NCCL for GPU-to-GPU, RDMA for cross-node
- **Operators**: Kubeflow Training Operator, Volcano for batch scheduling

## LLM Inference Serving

- **Serving frameworks**: vLLM, TensorRT-LLM, TGI (Text Generation Inference)
- **Optimization**: Quantization (INT8/FP8), speculative decoding, PagedAttention
- **Serving patterns**: API gateway with routing, model parallelism, multi-model serving

## MLOps Pipeline

| Stage | Tools |
|-------|-------|
| Data pipeline | Kubeflow Pipelines, Argo Workflows |
| Training | Kubeflow Training Operator, Ray |
| Experiment tracking | MLflow, Weights & Biases |
| Model registry | MLflow Model Registry |
| Deployment | KServe, Seldon Core |
| Monitoring | Prometheus + custom metrics, LLM observability tools |

## Cost Optimization

- Spot instances for fault-tolerant training jobs
- GPU autoscaling with Karpenter
- Model quantization to reduce inference GPU requirements
- Cost monitoring with Kubecost

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[concepts/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]
- [[concepts/scheduling-algorithm.md|Scheduling Algorithm]]
- [[concepts/resource-management.md|Resource Management]]
