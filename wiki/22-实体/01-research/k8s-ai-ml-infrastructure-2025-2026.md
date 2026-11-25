---
title: K8S Ai Ml Infrastructure 2025 2026
summary: 1. GPU Scheduling & Resource Management 2. Distributed Training Frameworks
  3. LLM Serving Infrastructure 4. ML Platforms on Kubernetes 5. AI Agent Infrastructure
  on K8S 6. Model Registry & Versioni...
category: entities
tags:
- k8s-ai-ml-infrastructure-2025-2026
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes AI/ML Infrastructure 2025-2026
# Research Report — Compiled 2026-05-24

## Table of Contents
1. [GPU Scheduling & Resource Management](#1-gpu-scheduling--resource-management)
2. [Distributed Training Frameworks](#2-distributed-training-frameworks)
3. [LLM Serving Infrastructure](#3-llm-serving-infrastructure)
4. [ML Platforms on Kubernetes](#4-ml-platforms-on-kubernetes)
5. [AI Agent Infrastructure on K8S](#5-ai-agent-infrastructure-on-k8s)
6. [Model Registry & Versioning](#6-model-registry--versioning)
7. [Feature Stores](#7-feature-stores)
8. [Vector Databases on K8S](#8-vector-databases-on-k8s)
9. [Key Trends & Synthesis](#9-key-trends--synthesis)
10. [Source URLs](#10-source-urls)

---

## 1. GPU Scheduling & Resource Management

### 1.1 Dynamic Resource Allocation (DRA) — GA in K8s v1.34

DRA is the most significant advancement in K8s GPU scheduling in 2025-2026.

Timeline:
- v1.26 (2022): DRA introduced as alpha
- v1.31 (2024): Major redesign
- v1.32 (2024): DRA core goes beta
- v1.33 (May 2025): New DRA features in alpha/beta; UX improvements
- v1.34 (Sep 2025): DRA GRADUATED TO GA — production-ready
  - DRA Consumable Capacity (sharing GPUs across workloads)
  - Pods Report DRA Resource Health
- v1.36 (May 2026): DRA continues maturing
  - Extends to native resources (memory, CPU)
  - ResourceClaims in PodGroups
  - More drivers: networking, storage, specialized hardware
  - Hardware-agnostic infrastructure direction

DRA vs Device Plugin:
- DRA provides a far more powerful and flexible API than the legacy Device Plugin framework
- Supports structured parameters, device classes, resource claims
- Enables vendor-neutral GPU/accelerator allocation
- Supports GPU partitioning (MIG), time-slicing, and fractional allocation natively

Source: https://kubernetes.io/blog/2025/09/01/kubernetes-v1-34-dra-updates/
Source: https://kubernetes.io/blog/2025/05/01/kubernetes-v1-33-dra-updates/
Source: https://kubernetes.io/blog/2026/05/07/kubernetes-v1-36-dra-136-updates/

### 1.2 NVIDIA GPU Operator

The NVIDIA GPU Operator remains the standard for NVIDIA GPU management on K8s.

Key capabilities (2025-2026):
- Automates NVIDIA driver, CUDA toolkit, DCGM, and device plugin deployment
- Integrates with DRA via nvidia-dra-driver for K8s v1.34+
- Supports MIG (Multi-Instance GPU) partitioning: A100/H100/H200 can be split into up to 7 instances
- Time-slicing support: enables GPU sharing among pods without hardware partitioning
- GPU Operator v25.x (2025): Better DRA integration, improved MIG management
- NVIDIA Network Operator for GPUDirect RDMA (multi-node training interconnect)

MIG vs Time-Slicing:
- MIG: Hardware-level isolation, guaranteed SLAs, best for production inference
- Time-slicing: Software-level sharing, higher utilization, some latency variance
- DRA (new): Unified framework that can orchestrate both approaches declaratively

Source: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/

### 1.3 Workload-Aware Scheduling (v1.35-1.36)

K8s v1.35 (Aug 2025): Introduces Workload-Aware Scheduling
K8s v1.36 (May 2026): Advancing Workload-Aware Scheduling
- Schedulers become aware of workload characteristics (CPU/memory/GPU profiles)
- Enables topology-aware placement for distributed training jobs
- PSI (Pressure Stall Information) Metrics GA in v1.36 for resource contention detection

Source: https://kubernetes.io/blog/2025/ (v1.35/v1.36 release blogs)

### 1.4 In-Place Pod Resource Scaling

- v1.35: In-Place Pod Resize GA (stable)
- v1.36: In-Place Vertical Scaling for Pod-Level Resources (beta)
- Critical for LLM serving: resize GPU/CPU allocation without restarting pods
- Enables elastic inference serving

---

## 2. Distributed Training Frameworks

### 2.1 PyTorch FSDP (Fully Sharded Data Parallel)

FSDP2 is the standard PyTorch distributed training approach in 2025-2026:
- Replaces FSDP1 with cleaner architecture
- Native integration with DTensor
- Per-parameter sharding (more flexible than per-module)
- Combined with torch.compile for maximum throughput
- Supports mixed precision (BF16/FP16) with gradient accumulation

PyTorch on K8s:
- TorchElastic (torchrun) for fault-tolerant training
- PyTorchJob CRD via Kubeflow Training Operator
- Automatic node failure recovery
- NCCL backend for GPU-to-GPU communication

Source: https://pytorch.org/docs/stable/fsdp.html

### 2.2 DeepSpeed

DeepSpeed remains essential for very large model training:
- ZeRO-3 (Infinity): Offloads optimizer states to NVMe
- DeepSpeed-Chat: RLHF training pipeline
- DeepSpeed-Ulysses: Efficient sequence parallelism for long-context LLMs
- MoE (Mixture of Experts) support
- DeepSpeed-FastGen: High-throughput inference (merged with vLLM direction)

On K8s:
- Integrates with Kubeflow Training Operator
- Launcher/worker pattern using K8s Job/PyTorchJob
- Requires high-bandwidth interconnect (NVLink, InfiniBand, GPUDirect)

Source: https://www.deepspeed.ai/

### 2.3 Megatron-LM

NVIDIA's framework for training trillion-parameter models:
- Tensor parallelism, pipeline parallelism, sequence parallelism
- Transformer Engine for FP8 training on H100/H200
- Megatron-Core: Modular library for custom model architectures
- Typically deployed on NVIDIA DGX clusters or cloud GPU instances on K8s

Source: https://github.com/NVIDIA/Megatron-LM

### 2.4 JAX/Flax on K8s

Google's alternative training stack:
- pjit for parallelism across TPU/GPU pods
- MaxText: Reference LLM implementation
- Orbax: Checkpoint management
- Popular on GKE with TPU v5 pods

### 2.5 Training Operator (Kubeflow)

The Kubeflow Training Operator (formerly tf-operator) now handles:
- PyTorchJob, TFJob, MPIJob, XGBoostJob, PaddleJob
- TrainJob (new unified API in 2025-2026)
- Elastic training support
- Gang scheduling via coscheduling plugin
- Integration with volcano scheduler for batch AI workloads

---

## 3. LLM Serving Infrastructure

### 3.1 vLLM

vLLM has become the dominant open-source LLM serving engine.

Key features (2025-2026):
- PagedAttention: Efficient KV cache management
- Continuous batching for high throughput
- Speculative decoding support
- Tensor parallelism for multi-GPU serving
- Prefix caching for repeated prompt patterns
- OpenAI-compatible API server
- Support for quantized models (GPTQ, AWQ, FP8, GGUF)
- Disaggregated prefill/decode architecture

vLLM on Kubernetes:
- Deployed as K8s Deployment with GPU resource requests
- v1.x (2025-2026): Stable API, production-ready
- Integrates with KServe for model serving
- Works with DRA for GPU allocation
- Horizontal scaling via KEDA or custom metrics

Source: https://docs.vllm.ai/
Source: https://github.com/vllm-project/vllm

### 3.2 TensorRT-LLM

NVIDIA's optimized inference engine:
- FP8 inference on H100/H200 (2x throughput vs FP16)
- In-flight batching for dynamic request handling
- Paged KV cache
- Multi-GPU tensor/pipeline parallelism
- FlashAttention integration
- Typically 30-50% faster than vLLM on NVIDIA hardware
- More complex setup, less flexible than vLLM

On K8s:
- Deployed via Triton Inference Server with TensorRT-LLM backend
- NVIDIA GPU Operator manages driver/runtime
- TRT-LLM + Triton + K8s is the NVIDIA-recommended production stack

Source: https://github.com/NVIDIA/TensorRT-LLM

### 3.3 NVIDIA Triton Inference Server

Production inference server supporting multiple backends:
- TensorRT-LLM backend for LLMs
- TensorRT, ONNX, PyTorch, Python backends
- Dynamic batching, model ensemble
- Model analyzer for performance optimization
- Prometheus metrics integration
- KServe-compatible

Source: https://docs.nvidia.com/deeplearning/triton-inference-server/

### 3.4 KServe (formerly KFServing)

The standard K8s model serving framework:
- KNative-based serverless inference with scale-to-zero
- Transformer/Explainer/Predictor pattern
- v0.14+ (2025): Improved LLM serving support
  - vLLM runtime integration
  - ModelMesh for high-density model serving
  - Raw deployment mode (no KNative dependency)
- InferenceService CRD for declarative model deployment
- GPU autoscaling based on inference metrics
- A/B testing and canary deployments for models

Source: https://kserve.github.io/website/

### 3.5 SGLang

Emerging LLM serving framework (2025-2026):
- RadixAttention for efficient prefix caching
- Compressed finite state machines for structured output
- Faster than vLLM for some workloads (especially structured generation)
- Growing adoption for agent/tool-calling use cases

Source: https://github.com/sgl-project/sglang

### 3.6 Ollama / Local LLM

- Simple local LLM serving, gaining K8s deployment traction
- Helm charts available for K8s deployment
- Lightweight alternative for smaller models
- Often used for development/staging, not production

---

## 4. ML Platforms on Kubernetes

### 4.1 Kubeflow (v1.10 / "master" branch)

Kubeflow remains the comprehensive ML platform for K8s.

Current state (2025-2026):
- Version numbering: v1.10 is latest stable (not "2.0" — there is no Kubeflow 2.0)
- Components:
  - Kubeflow Pipelines (KFP) v2: New pipeline spec, improved caching
  - Training Operator: PyTorchJob, TrainJob (unified)
  - KServe integration for model serving
  - Katib for hyperparameter tuning
  - Notebooks (Jupyter) with GPU support
  - Spark Operator integration
  - Kubeflow SDK (Python SDK for pipelines)

Key 2025-2026 changes:
- GenAI Use Cases added to documentation
- Training Operator gaining TrainJob unified API
- Improved GPU scheduling integration with DRA
- Better multi-tenancy support
- Spark Operator as first-class component

Source: https://www.kubeflow.org/docs/started/introduction/
Source: https://www.kubeflow.org/docs/ (version: master / v1.10)

### 4.2 MLflow

The standard ML experiment tracking and model registry:
- MLflow 2.x (2025): Enhanced LLM evaluation
  - mlflow.evaluate() for LLMs
  - GenAI flavor support
  - AI Gateway for unified LLM access
  - Tracing for LLM observability
- On K8s:
  - Deployed via Helm chart
  - PostgreSQL/MySQL backend
  - S3/GCS/Azure Blob for artifact storage
  - Used alongside Kubeflow or standalone

Source: https://mlflow.org/

### 4.3 Ray on Kubernetes

Ray is the dominant distributed compute framework for AI workloads.

Ray on K8s (2025-2026):
- KubeRay Operator: The official way to run Ray on K8s
  - RayCluster, RayJob, RayService CRDs
  - Autoscaling with K8s cluster autoscaler integration
  - GPU scheduling support
- Use cases:
  - Distributed data processing (Ray Data)
  - Model training (Ray Train)
  - Hyperparameter tuning (Ray Tune)
  - Model serving (Ray Serve) — LLM serving with continuous batching
  - Reinforcement learning (RLlib)
  - Batch inference
- Ray Serve on K8s:
  - Competitive with vLLM for some workloads
  - Multi-model serving
  - Autoscaling based on request queue

Ray 2.x (current: 2.55.1):
- Ray Data for ML data pipelines
- Ray Train for distributed training
- Improved GPU scheduling
- Better fault tolerance

Source: https://docs.ray.io/en/latest/cluster/kubernetes/index.html
Source: https://docs.ray.io/en/latest/

### 4.4 Metaflow

Netflix's ML framework, growing K8s adoption:
- Step-based workflow definition
- K8s execution backend
- Integration with external orchestrators

### 4.5 Argo Workflows + ML

Argo Workflows used as ML pipeline backbone:
- DAG-based pipeline execution
- GPU resource management
- Used by some teams instead of Kubeflow Pipelines
- Hera SDK for Python workflow definition

---

## 5. AI Agent Infrastructure on K8S

### 5.1 Agent Sandbox (K8s Blog 2026)

K8s blog (2026): "Running Agents on Kubernetes with Agent Sandbox"
- Official guidance on running AI agents on K8s
- Sandboxing for untrusted agent code execution
- Security boundaries for agent tool execution

Source: https://kubernetes.io/blog/ (2026 posts)

### 5.2 AI Gateway Working Group

K8s announced "Announcing the AI Gateway Working Group" (2026)
- Focus on standardizing AI/LLM traffic management in K8s
- Gateway API extensions for AI workloads
- Rate limiting, model routing, prompt management at the gateway layer

Source: https://kubernetes.io/blog/ (2026 posts)

### 5.3 Agent Infrastructure Patterns

Emerging patterns for AI agents on K8s:
- Pod-per-agent: Each agent session gets its own pod
- Shared inference backend: Agents share vLLM/Triton serving endpoints
- Tool execution: Agents execute tools via K8s Jobs or dedicated sandboxes
- Memory: Vector DB + session state in Redis/PostgreSQL
- Orchestration: LangGraph, CrewAI, AutoGen deployed as K8s services
- Human-in-the-loop: K8s-native approval workflows

### 5.4 Agent Frameworks on K8s

- LangChain/LangGraph: Deployed as Python services on K8s
- CrewAI: Multi-agent orchestration, K8s deployment
- AutoGen (Microsoft): Group chat agents
- Semantic Kernel (Microsoft): Enterprise agent framework
- OpenAI Agents SDK: Lightweight, K8s-deployable

---

## 6. Model Registry & Versioning

### 6.1 MLflow Model Registry

- Most widely adopted model registry
- Model versioning, stage transitions (None → Staging → Production → Archived)
- Model signatures and input examples
- Integration with CI/CD pipelines
- LLM model support (transformers flavor)

### 6.2 Kubeflow Model Registry

- K8s-native model registry
- CRD-based model artifacts
- Integration with KServe for serving
- Growing ecosystem

### 6.3 Other Options

- Weights & Biases (W&B): Experiment tracking + model registry (SaaS)
- Neptune.ai: Metadata store + model registry
- DVC (Data Version Control): Git-based model versioning
- Hugging Face Hub: De facto standard for open-weight model distribution
- OCI registries: Models stored as OCI artifacts (ORAS)

---

## 7. Feature Stores

### 7.1 Feast (Open Source)

- Most popular open-source feature store
- K8s deployment via Helm
- Online store (Redis/DynamoDB) + offline store (Parquet/BigQuery/Snowflake)
- Point-in-time joins for training data
- Feature server for real-time serving

Source: https://feast.dev/

### 7.2 Tecton

- Managed feature platform (SaaS)
- Real-time feature computation
- K8s integration via API

### 7.3 Hopsworks

- Open-source ML platform with built-in feature store
- K8s deployment
- Feature groups, time-travel queries

---

## 8. Vector Databases on K8S

### 8.1 Milvus

- Leading open-source vector database
- K8s deployment via Helm (milvus-helm)
- Supports billion-scale vector search
- GPU-accelerated indexing (GPU_IVF_FLAT, GPU_IVF_PQ)
- Hybrid search (vector + scalar filtering)
- Distributed architecture: proxy, query node, data node, index node

Source: https://milvus.io/

### 8.2 Qdrant

- High-performance vector database
- K8s Helm chart
- Filtering during search (not post-filter)
- Payload-based filtering
- Rust-based, memory-efficient

Source: https://qdrant.tech/

### 8.3 Weaviate

- Vector search engine with built-in ML models
- K8s Helm chart
- Multi-modal search
- GraphQL API
- Module system for embedding models

Source: https://weaviate.io/

### 8.4 Chroma

- Lightweight embedding database
- Growing K8s deployment support
- Popular for RAG prototyping

### 8.5 pgvector (PostgreSQL extension)

- Vector similarity search in PostgreSQL
- Used with CloudNativePG operator on K8s
- Simple option for teams already using PostgreSQL

### 8.6 Pinecone (SaaS)

- Managed vector database
- K8s integration via API
- No self-hosted K8s option

---

## 9. Key Trends & Synthesis

### 2025-2026 Major Trends

1. DRA is the GPU scheduling story: GA in v1.34, maturing in v1.35/v1.36.
   DRA replaces Device Plugin for complex GPU allocation patterns.

2. vLLM dominates LLM serving: Standard open-source inference engine.
   SGLang emerging as alternative for structured output workloads.

3. In-place pod resize (GA): Enables elastic inference without restarts.

4. AI Gateway Working Group: K8s investing in AI-native networking.

5. Agent Sandbox: K8s providing first-class support for AI agent execution.

6. Kubeflow consolidates: v1.10 is mature, no "2.0" — incremental improvements.
   Training Operator adding unified TrainJob API.

7. KubeRay is the standard for Ray on K8s: Ray Serve competitive for LLM serving.

8. Workload-Aware Scheduling: K8s becoming smarter about GPU placement.

9. Vector databases mature: Milvus, Qdrant, Weaviate all have production K8s
   deployments. RAG is the standard pattern.

10. GPU sharing becomes first-class: DRA + MIG + time-slicing + fractional
    allocation all under one API. Critical for inference cost optimization.

### Architecture Pattern: Production LLM Platform on K8s (2025-2026)

```
┌─────────────────────────────────────────────────┐
│                  Gateway Layer                    │
│  Gateway API + AI Gateway (rate limit, routing)  │
├─────────────────────────────────────────────────┤
│               Serving Layer                       │
│  KServe InferenceService ──► vLLM / TensorRT-LLM │
│  Ray Serve for complex pipelines                  │
│  Triton for multi-model serving                   │
├─────────────────────────────────────────────────┤
│               Platform Layer                      │
│  Kubeflow Pipelines │ MLflow │ Katib             │
│  Training Operator (PyTorchJob/TrainJob)          │
├─────────────────────────────────────────────────┤
│               Data Layer                          │
│  Vector DB (Milvus/Qdrant) │ Feature Store (Feast)│
│  Object Storage (S3) │ Model Registry (MLflow)    │
├─────────────────────────────────────────────────┤
│               Infrastructure Layer                │
│  NVIDIA GPU Operator │ DRA │ MIG │ Time-slicing  │
│  KubeRay │ Volcano │ Coscheduling                 │
│  K8s v1.34+ with DRA GA                          │
└─────────────────────────────────────────────────┘
```

---

## 10. Source URLs

### Kubernetes Official
- DRA GA (v1.34): https://kubernetes.io/blog/2025/09/01/kubernetes-v1-34-dra-updates/
- DRA v1.33: https://kubernetes.io/blog/2025/05/01/kubernetes-v1-33-dra-updates/
- DRA v1.36: https://kubernetes.io/blog/2026/05/07/kubernetes-v1-36-dra-136-updates/
- DRA Consumable Capacity: https://kubernetes.io/blog/2025/09/18/kubernetes-v1-34-dra-consumable-capacity/
- DRA Resource Health: https://kubernetes.io/blog/2025/09/17/kubernetes-v1-34-pods-report-dra-resource-health/
- Workload-Aware Scheduling: https://kubernetes.io/blog/2025/ (v1.35 release)
- In-Place Pod Resize GA: https://kubernetes.io/blog/2025/ (v1.35 release)
- Agent Sandbox: https://kubernetes.io/blog/ (2026)
- AI Gateway WG: https://kubernetes.io/blog/ (2026)

### NVIDIA
- GPU Operator: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/
- DRA Driver: https://github.com/NVIDIA/k8s-dra-driver
- TensorRT-LLM: https://github.com/NVIDIA/TensorRT-LLM
- Triton: https://docs.nvidia.com/deeplearning/triton-inference-server/

### LLM Serving
- vLLM: https://docs.vllm.ai/ / https://github.com/vllm-project/vllm
- KServe: https://kserve.github.io/website/
- SGLang: https://github.com/sgl-project/sglang

### ML Platforms
- Kubeflow: https://www.kubeflow.org/docs/started/introduction/
- MLflow: https://mlflow.org/
- Ray on K8s: https://docs.ray.io/en/latest/cluster/kubernetes/index.html
- KubeRay: https://ray-project.github.io/kuberay/

### Training
- DeepSpeed: https://www.deepspeed.ai/
- Megatron-LM: https://github.com/NVIDIA/Megatron-LM
- PyTorch FSDP: https://pytorch.org/docs/stable/fsdp.html

### Vector Databases
- Milvus: https://milvus.io/
- Qdrant: https://qdrant.tech/
- Weaviate: https://weaviate.io/

### Feature Stores
- Feast: https://feast.dev/

---

*Report compiled from Kubernetes official blog posts, project documentation,
and ecosystem knowledge as of May 2026.*


<!-- risk-assessed -->
