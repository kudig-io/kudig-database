---
category: synthesis
tags:
  - ai
  - ml
  - gpu
  - llm
  - k8s
  - research
created: 2026-05-24
updated: 2026-05-24
---

# Research: Kubernetes AI-ML Infrastructure 2025-2026

## 概述

2025-2026年间，Kubernetes作为AI/ML工作负载的基础设施平台经历了根本性转变。Dynamic Resource Allocation (DRA) 正式GA标志着GPU调度从"黑魔法"走向声明式API；vLLM成为事实上的LLM推理引擎标准，其PagedAttention架构重新定义了GPU内存管理范式；Kubeflow v2与Ray on K8S的成熟使端到端ML管道具备了生产级可靠性。这一时期的核心叙事是：**Kubernetes不再仅仅是容器编排器，而是AI基础设施的控制平面**。

## 核心发现

1. **DRA GA重塑GPU调度**：Kubernetes 1.31+中DRA正式GA，通过ResourceClaim和DeviceClass对象实现了GPU资源的声明式分配。多厂商GPU（NVIDIA、AMD、Intel）可统一声明，支持MIG/MPS/vGPU等细粒度切分，彻底取代了Device Plugin时代的hack式集成。

2. **vLLM统治LLM推理层**：vLLM 0.6+在K8S生态中成为压倒性选择，其与Knative/KEDA的集成实现了基于队列深度的自动扩缩容。TensorRT-LLM和Ollama退居特定场景（极致性能/边缘部署），SGLang作为后起之秀在结构化生成领域获得份额。

3. **Kubeflow v2 + Ray统一ML管道**：Kubeflow Pipelines v2采用Argo Workflows作为底层引擎，与Ray集群原生集成，覆盖从特征工程到模型服务的全链路。Ray Cluster CRD成为K8S上分布式计算的通用原语，取代了早期Spark on K8S在ML场景的角色。

4. **GPU共享与MPS成为生产标配**：通过DRA + NVIDIA MPS的组合，单GPU多模型推理成为成本优化的关键策略。MIG（Multi-Instance GPU）在A100/H100上的应用使5-7个独立推理实例共享单卡成为可能，GPU利用率从30-40%提升至70-80%。

5. **模型服务标准化（Model Mesh/Serverless）**：KServe和Seldon Core v2成为模型服务标准，结合Knative实现scale-to-zero。Open Inference Protocol的采纳使模型服务API脱离底层引擎绑定，推理端点具备了可移植性。

6. **AI Gateway层浮现**：以Envoy AI Gateway和Kong AI Gateway为代表的专用网关层出现，提供token计量、语义缓存、模型路由和A/B测试等LLM专属能力，填补了传统API Gateway在AI场景的能力空白。

## 核心概念

相关核心概念详见 [[concepts/k8s-ai-ml-infrastructure]]，涵盖：

- DRA GPU 调度 — Dynamic Resource Allocation与GPU资源管理
- vLLM 推理 — vLLM架构与K8S集成模式
- Kubeflow — Kubeflow Pipelines v2架构演进
- Ray on K8S — Ray分布式计算在K8S上的部署模型
- 模型服务标准 — Open Inference Protocol与模型服务标准
- AI Gateway — AI Gateway与推理流量管理

## 矛盾与争议

| 议题 | 立场A | 立场B |
|------|-------|-------|
| DRA vs. 专用AI调度器 | DRA是标准答案，应统一调度 | Run:ai/CoreWeave等专用方案提供更优的排队与抢占策略 |
| vLLM vs. TensorRT-LLM | vLLM灵活性足以覆盖大多数场景 | TensorRT-LLM在确定性延迟场景仍显著优于vLLM |
| K8S vs. 专用AI平台 | K8S + DRA足够支撑AI基础设施 | MosaicML/OctoML等认为K8S开销过重，应走更薄的抽象层 |
| GPU共享安全性 | MPS/MIG在K8S隔离下足够安全 | 多租户GPU共享仍存在侧信道攻击风险，需硬件级TEE |

## 来源

- Kubernetes SIG-Node DRA KEP (KEP-4381), GA milestone 2025
- vLLM Project Documentation & Benchmark Reports, v0.6.x series
- Kubeflow v2 Release Notes & Architecture Decision Records
- Ray on Kubernetes Best Practices Guide, Anyscale 2025
- CNCF AI/ML Working Group Landscape Report Q1 2026
- NVIDIA Multi-Instance GPU Technical Brief, H100/H200 series
- KServe v0.13+ Release Notes & Open Inference Protocol Spec

---

## 跨域关联

- [[concepts/container-runtime-evolution]] — 容器运行时演进（Wasm、gVisor）为 AI/ML 推理工作负载提供更高效的隔离与执行环境
- [[concepts/k8s-networking-evolution]] — 高性能网络（RDMA、GPUDirect RDMA）是分布式 AI 训练集群的关键基础设施
- [[concepts/storage-performance-optimization]] — 存储性能优化（并行文件系统、本地 NVMe 缓存）直接影响模型训练 I/O 吞吐
- [[concepts/finops-greenops-practices]] — GPU 资源成本管理与绿色计算是 AI/ML 基础设施可持续运营的核心议题
