---
title: K8s AI 基础设施
description: Kubernetes 原生 AI 基础设施知识目录 — GPU 调度、推理服务、训练平台、向量数据库、高性能网络
summary: K8s AI 基础设施完整知识体系：GPU Operator 与共享模式、vLLM/Triton/KServe 推理服务、KubeRay/Volcano 训练调度、向量数据库、RDMA 网络、微调基础设施、AI 可观测性
category: AI基础设施
tags:
- ai
- gpu
- kubernetes
- inference
- training
- mlops
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- AI 工程师
- MLOps 工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- "K8s AI 基础设施有哪些内容"
- "AI 平台知识库目录"
trigger_keywords:
- AI基础设施
- GPU
- 推理服务
- 训练平台
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# K8s AI 基础设施

本目录收录 Kubernetes 原生 AI 基础设施的完整知识体系，覆盖从 GPU 硬件管理到上层 AI 应用的全栈技术。适用于 K8s 1.28-1.32 版本。

## GPU 与设备管理

- [[15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-sharing-patterns|GPU Operator 部署与管理]] — NVIDIA GPU Operator 全组件部署、驱动管理、升级策略与生产配置
- [[15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-sharing-patterns|GPU 共享模式：MIG 与时间切片]] — MIG 多实例 GPU 切分、Time-Slicing 共享、vGPU 方案对比与调度配置
- [[15-AI基础设施/05-K8s-AI基础设施/17-cdi-device-plugin-framework.md|CDI 与 Device Plugin 框架]] — Device Plugin gRPC 架构、CDI 规范、DRA 动态资源分配与设备拓扑感知调度

## 推理服务

- [[15-AI基础设施/05-K8s-AI基础设施/03-vllm-inference-serving-production|vLLM 生产部署]] — vLLM 推理引擎 K8s 部署、PagedAttention 调优、多模型管理与 OpenAI 兼容 API
- [[15-AI基础设施/05-K8s-AI基础设施/04-tgi-triton-tensorrt-serving|Triton Inference Server]] — NVIDIA Triton 多框架推理、模型仓库、动态批处理与 Ensemble Pipeline
- [[15-AI基础设施/05-K8s-AI基础设施/05-kserve-model-serving-platform|KServe 模型服务平台]] — KServe InferenceService、Canary 发布、模型解释性与多框架 Predictor
- [[15-AI基础设施/05-K8s-AI基础设施/14-model-serving-autoscaling-keda.md|推理服务自动伸缩]] — KEDA/Knative 驱动的推理伸缩：队列驱动、GPU 利用率、scale-to-zero 与冷启动优化
- [[15-AI基础设施/05-K8s-AI基础设施/16-sglang-lmdeploy-inference.md|SGLang 与 LMDeploy 推理引擎]] — SGLang RadixAttention、LMDeploy TurboMind W4A16 量化、与 vLLM 性能对比

## 训练与调度

- [[15-AI基础设施/05-K8s-AI基础设施/07-training-operators-volcano-mpi|Volcano 批调度器]] — Gang Scheduling、队列管理、公平共享策略与分布式训练任务编排
- [[15-AI基础设施/05-K8s-AI基础设施/06-kuberay-distributed-computing|KubeRay 分布式训练]] — KubeRay Operator、RayJob/RayCluster CRD、Ray Train 与 GPU 集群弹性训练
- [[15-AI基础设施/05-K8s-AI基础设施/11-finetuning-peft-lora-deepspeed|微调基础设施]] — LoRA/QLoRA 微调管道、数据集管理、训练任务模板与模型评估流水线

## 数据与存储

- [[15-AI基础设施/05-K8s-AI基础设施/09-vector-database-k8s-milvus-qdrant|向量数据库 K8s 部署]] — Milvus/Qdrant/Weaviate 在 K8s 上的生产部署、分片策略与 RAG 集成
- [[15-AI基础设施/05-K8s-AI基础设施/10-rdma-infiniband-gpudirect-networking|RDMA 高性能网络]] — RoCEv2/InfiniBand 配置、Multus 多网卡、NCCL 通信优化与网络故障排查

## 平台与治理

- [[15-AI基础设施/05-K8s-AI基础设施/13-ai-observability-arize-phoenix|AI 可观测性]] — DCGM Exporter、推理指标（TTFT/TPS）、训练监控、Grafana Dashboard 与告警规则
- [[15-AI基础设施/01-基础设施/11-ai-security-model-protection|AI 工作负载安全]] — 模型安全、供应链安全、Runtime 隔离、Secret 管理与合规审计
- [[15-AI基础设施/05-K8s-AI基础设施/15-gpu-cost-attribution-multitenant.md|GPU 成本分摊与多租户 AI 平台]] — OpenCost GPU 归因、ResourceQuota 配额、多租户隔离与 showback/chargeback 计费
- [[15-AI基础设施/05-K8s-AI基础设施/18-ai-platform-architecture-reference.md|企业 AI 平台参考架构]] — 五层分层架构、技术选型矩阵、数据流设计、建设路线图

## 阅读建议

| 角色 | 推荐路径 |
|------|---------|
| AI 工程师（入门） | 01 → 03 → 05 → 09 |
| MLOps 工程师 | 01 → 06 → 07 → 08 → 13 |
| SRE / 平台工程 | 01 → 02 → 10 → 11 → 14 → 16 |
| 架构师 | 17 → 14 → 16 → 13 → 05 |

## Related

- [[15-AI基础设施/01-基础设施/01-ai-infrastructure-overview.md|AI 基础设施概述]]
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]
- [[15-AI基础设施/01-基础设施/17-llm-inference-serving.md|LLM 推理服务]]
- [[15-AI基础设施/01-基础设施/32-mlops-pipeline.md|MLOps Pipeline]]

## 文档

- [[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving.md|02-gpu-cluster-scheduling-inference-serving]]
