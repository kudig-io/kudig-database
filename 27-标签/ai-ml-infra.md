---
title: ai-ml-infra
description: AI/ML 基础设施标签枢纽 — 涵盖 GPU 调度、分布式训练、LLM 推理、MLOps、AI Agent、向量数据库、模型服务等全部 AI 基础设施知识
category: tag-index
tags:
- ai-ml-infra
- mlops
- llm
- inference
- training
- agent
tier: core
difficulty: intermediate-to-advanced
domain: ai-infrastructure
created: '2026-07-11'
last_updated: '2026-07-21'
---

# ai-ml-infra Tag Hub

> AI/ML 基础设施页面 — GPU 调度、分布式训练、LLM 推理、MLOps、AI Agent、向量数据库等。

## 核心定义

**AI/ML 基础设施**是在 Kubernetes 上构建 AI 工作负载运行环境的系统化实践，涵盖 GPU 资源管理、分布式训练编排、模型推理服务、MLOps 流水线、AI Agent 运行时等核心能力。

### AI 基础设施能力矩阵

| 能力域 | 描述 | 关键工具 |
|--------|------|----------|
| GPU 调度 | GPU 资源分配与管理 | GPU Operator, Device Plugin |
| 分布式训练 | 多节点/多卡训练编排 | Kubeflow Training, PyTorchJob |
| 模型推理 | 在线/批量推理服务 | vLLM, Triton, KServe |
| MLOps | ML 生命周期管理 | Kubeflow, MLflow, Argo |
| AI Agent | 智能体运行时 | LangChain, AgentScope |
| 向量数据库 | 语义检索与 RAG | Milvus, Weaviate, Qdrant |


## AI 基础设施 (AI Infrastructure)

- [[15-AI基础设施/README|AI 基础设施索引]]
- [[15-AI基础设施/01-基础设施/01-ai-infrastructure-overview|AI 基础设施概览]]
- [[15-AI基础设施/01-基础设施/02-ai-ml-workloads|AI/ML 工作负载]]
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management|GPU 调度管理]]
- [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm|GPU 监控 DCGM]]
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks|分布式训练框架]]
- [[15-AI基础设施/01-基础设施/06-ai-data-pipeline|AI 数据管道]]
- [[15-AI基础设施/01-基础设施/07-ai-experiment-management|AI 实验管理]]
- [[15-AI基础设施/01-基础设施/08-automl-hyperparameter-tuning|AutoML 超参数调优]]
- [[15-AI基础设施/01-基础设施/09-model-registry|模型注册]]
- [[15-AI基础设施/01-基础设施/10-model-deployment-management|模型部署管理]]
- [[15-AI基础设施/01-基础设施/11-ai-security-model-protection|AI 安全与模型保护]]
- [[15-AI基础设施/01-基础设施/12-ai-cost-analysis-finops|AI 成本分析 FinOps]]
- [[15-AI基础设施/01-基础设施/13-ai-platform-observability|AI 平台可观测性]]
- [[15-AI基础设施/01-基础设施/14-troubleshooting-performance|AI 性能故障排查]]
- [[15-AI基础设施/01-基础设施/38-ai-ml-ops-runbook|AI/ML Ops Runbook]]
- [[15-AI基础设施/01-基础设施/39-kubeflow-ai-platform-guide|Kubeflow AI 平台指南]]
- [[15-AI基础设施/00-总览/01-production-readiness-operations-guide|AI 基础设施生产就绪指南]]

## LLM 推理与服务 (LLM Inference & Serving)

- [[15-AI基础设施/01-基础设施/15-llm-data-pipeline|LLM 数据管道]]
- [[15-AI基础设施/01-基础设施/16-llm-finetuning|LLM 微调]]
- [[15-AI基础设施/01-基础设施/17-llm-inference-serving|LLM 推理服务部署]]
- [[15-AI基础设施/01-基础设施/18-llm-serving-architecture|LLM 服务架构]]
- [[15-AI基础设施/01-基础设施/19-llm-quantization|LLM 量化]]
- [[15-AI基础设施/01-基础设施/20-vector-database-rag|向量数据库与 RAG]]
- [[15-AI基础设施/01-基础设施/21-multimodal-models|多模态模型]]
- [[15-AI基础设施/01-基础设施/22-llm-privacy-security|LLM 隐私安全]]
- [[15-AI基础设施/01-基础设施/23-llm-cost-monitoring|LLM 成本监控]]
- [[15-AI基础设施/01-基础设施/24-llm-model-versioning|LLM 模型版本管理]]
- [[15-AI基础设施/01-基础设施/25-llm-observability|LLM 可观测性]]

## AI Agent

- [[15-AI基础设施/02-AI-Agents/01-ai-agent-fundamentals|AI Agent 基础]]
- [[15-AI基础设施/02-AI-Agents/02-llm-foundation-models|LLM 基础模型]]
- [[15-AI基础设施/02-AI-Agents/03-agent-frameworks-comparison|Agent 框架对比]]
- [[15-AI基础设施/02-AI-Agents/04-rag-knowledge-retrieval|RAG 知识检索]]
- [[15-AI基础设施/02-AI-Agents/05-tool-use-function-calling|工具调用 Function Calling]]
- [[15-AI基础设施/02-AI-Agents/06-multi-agent-orchestration|多 Agent 编排]]
- [[15-AI基础设施/02-AI-Agents/07-memory-context-management|记忆上下文管理]]
- [[15-AI基础设施/02-AI-Agents/08-agent-evaluation-observability|Agent 评估可观测性]]
- [[15-AI基础设施/02-AI-Agents/09-production-deployment-guide|Agent 生产部署指南]]
- [[15-AI基础设施/02-AI-Agents/10-security-guardrails|Agent 安全护栏]]
- [[15-AI基础设施/02-AI-Agents/12-enterprise-case-studies|企业案例研究]]

## Agent 运行时 (Agent Runtime)

- [[15-AI基础设施/03-Agent运行时/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[15-AI基础设施/03-Agent运行时/02-llamaindex-data-agent|LlamaIndex Data Agent]]
- [[15-AI基础设施/03-Agent运行时/03-crewai-multi-agent-framework|CrewAI 多 Agent 框架]]
- [[15-AI基础设施/03-Agent运行时/04-autogen-microsoft-agent|AutoGen Microsoft Agent]]
- [[15-AI基础设施/03-Agent运行时/05-dify-agent-platform|Dify Agent 平台]]
- [[15-AI基础设施/03-Agent运行时/12-agent-sandbox-isolation|Agent 沙箱隔离]]
- [[15-AI基础设施/03-Agent运行时/13-agent-observability-langfuse|Agent 可观测性 Langfuse]]
- [[15-AI基础设施/03-Agent运行时/20-agent-multi-tenancy|Agent 多租户]]
- [[15-AI基础设施/03-Agent运行时/21-agent-runtime-architecture-overview|Agent 运行时架构概览]]

## AI 编码 (AI Coding)

- [[15-AI基础设施/04-AI编码/README|AI 编码索引]]
- [[15-AI基础设施/04-AI编码/13-opencode-overview-architecture|OpenCode 概览架构]]
- [[15-AI基础设施/04-AI编码/16-opencode-agents-system|OpenCode Agents 系统]]

## MLOps

- [[15-AI基础设施/01-基础设施/32-mlops-pipeline|MLOps 管道]]
- [[15-AI基础设施/01-基础设施/33-model-explainability|模型可解释性]]
- [[15-AI基础设施/01-基础设施/34-federated-learning|联邦学习]]
- [[15-AI基础设施/01-基础设施/35-model-drift-monitoring|模型漂移监控]]
- [[15-AI基础设施/01-基础设施/36-ai-platform-observability-enhanced|AI 平台增强可观测性]]
- [[15-AI基础设施/01-基础设施/37-agent-sandbox-security|Agent 沙箱安全]]

## 清单模式 (Manifest Patterns)

- [[03-清单模式/07-AI-ML模式/01-gpu-pod-scheduling|GPU Pod 调度]]
- [[03-清单模式/07-AI-ML模式/02-mig-partitioning-manifests|MIG 分区清单]]
- [[03-清单模式/07-AI-ML模式/03-vllm-deployment-manifest|vLLM 部署清单]]
- [[03-清单模式/07-AI-ML模式/04-triton-deployment-manifest|Triton 部署清单]]
- [[03-清单模式/07-AI-ML模式/05-training-job-pytorch|PyTorch 训练任务]]
- [[03-清单模式/07-AI-ML模式/06-mpi-operator-patterns|MPI Operator 模式]]
- [[03-清单模式/07-AI-ML模式/07-model-serving-hpa|模型服务 HPA]]
- [[03-清单模式/07-AI-ML模式/08-gpu-sharing-time-slicing|GPU 共享时间切片]]

## 概念 (Concepts)

- [[22-概念/12-研究/k8s-ai-ml-infrastructure|K8s AI/ML 基础设施]]
- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads|GPU 调度与 AI 工作负载]]
- [[22-概念/12-研究/ai-agent-ops-patterns|AI Agent 运维模式]]
- [[22-概念/06-可观测性/ai-ml-observability|AI/ML 可观测性]]
- [[22-概念/13-research-2025-2026/01-AI-ML-Infrastructure|AI/ML 基础设施研究]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/structural-10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|AI/ML 工作负载排障]]
- [[19-故障诊断/04-高级排障/structural-09-dra-troubleshooting|DRA 排障]]
- [[26-技能/03-节点/gpu/诊断排障/ts-ai-ml-workloads.md|AI/ML 工作负载排障技能]]
- [[26-技能/03-节点/gpu/gpu-fta|GPU FTA]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/specialized-workloads/kubeflow|Kubeflow]]
- [[17-系统基础/06-知识字典/specialized-workloads/ray|Ray]]
- [[17-系统基础/06-知识字典/specialized-workloads/kserve-model-serving|KServe 模型服务]]
- [[17-系统基础/06-知识字典/specialized-workloads/mlops-pipelines-and-model-registry|MLOps 管道与模型注册]]
- [[17-系统基础/06-知识字典/specialized-workloads/llm-inference-optimization|LLM 推理优化]]
- [[17-系统基础/06-知识字典/specialized-workloads/gpu-resource-management-and-partitioning|GPU 资源管理与分区]]
- [[17-系统基础/06-知识字典/specialized-workloads/kueue-job-queue-management|Kueue 任务队列管理]]
- [[17-系统基础/06-知识字典/specialized-workloads/hpc-and-bioinformatics|HPC 与生物信息学]]
- [[17-系统基础/06-知识字典/operations/holmesgpt|HolmesGPT]]
- [[17-系统基础/06-知识字典/operations/k8sgpt|K8sGPT]]

## 研究 (Research)

- [[25-研究/01-AI与边缘/gpu-sharing-scheduling|GPU 共享与调度]]

## 实体 (Entities)

- [[23-实体/15-参考与索引/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]]
- [[23-实体/15-参考与索引/k8s-ai-infrastructure|K8s AI Infrastructure]]
- [[23-实体/11-AI与边缘/kubeflow|Kubeflow]]
- [[23-实体/11-AI与边缘/kserve|KServe]]
- [[23-实体/11-AI与边缘/kaito|KAITO]]
- [[23-实体/11-AI与边缘/hami|HAMI]]
- [[23-实体/09-编排调度/koordinator|Koordinator]]
- [[23-实体/09-编排调度/volcano|Volcano]]
- [[23-实体/15-参考与索引/cncf-edge-ai|CNCF Edge AI]]

## 应用架构 (Application Architecture)

- [[04-应用模式/02-行业架构/08-ai-ml-inference-architecture|AI/ML 推理架构]]
- [[04-应用模式/02-行业架构/63-industrial-visual-inspection|工业视觉检测]]
- [[04-应用模式/02-行业架构/64-ai-drug-discovery|AI 药物研发]]
- [[04-应用模式/02-行业架构/65-autonomous-driving-sim|自动驾驶仿真]]

## 生态参考 (Ecosystem)

- [[21-生态参考/03-领域索引/ai-gpu-index|AI/GPU 索引]]
- [[21-生态参考/02-论文/17-kubernetes-aiml-gpu-scheduling-llm-inference|AI/ML GPU 调度与 LLM 推理]]
- [[21-生态参考/02-论文/25-gke-autopilot-google-cloud-ai-infrastructure|GKE Autopilot 与 Google Cloud AI]]

## 综合 (Synthesis)

- [[24-综合/01-AI与机器学习/gpu-scheduling-cost|GPU 调度与成本]]

## AI/ML 基础设施全景

### AI 基础设施组件

| 组件 | 功能 | 工具 |
|---|---|---|
| GPU 调度 | 资源分配 | Volcano, Kueue |
| 训练框架 | 分布式训练 | PyTorch, TensorFlow |
| 推理服务 | 模型部署 | KServe, Triton |
| 数据管理 | 数据集编排 | JuiceFS, Alluxio |

### MLOps 流程

```
数据准备 → 训练 → 评估 → 部署 → 监控 → 迭代
```

## 面试要点

1. **Q：K8s 上运行 AI 工作负载的挑战？**
   A：GPU 资源管理、大文件存储、长时任务容错、高带宽网络、资源调度。

2. **Q：GPU 调度策略？**
   A：整卡分配、GPU 共享、拓扑感知、抢占调度、队列管理。

3. **Q：推理服务的高可用设计？**
   A：多副本、自动扩缩、健康检查、流量控制、模型版本管理。

## Related Tags

- [[27-标签/gpu|gpu]]
- [[27-标签/k8s|k8s]]
- [[27-标签/observability|observability]]
- [[27-标签/production|production]]
