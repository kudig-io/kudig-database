---
title: AI / GPU 基础设施知识图谱索引
description: '## AI / GPU 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- ai
- gpu
- mlops
- kubeflow
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- AI GPU 知识图谱 是什么
- AI GPU 基础设施 相关文档
trigger_keywords:
- AI
- GPU
- 知识图谱
- index
---

# AI / GPU 基础设施知识图谱索引

> 知识图谱：按关键字 **ai-gpu** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### AI 基础设施

- [AI基础设施架构](./domain-11-ai-infra/01-ai-infrastructure-overview.md)
- [132 - AI/ML工作负载运维 (AI/ML Workloads Operations)](./domain-11-ai-infra/02-ai-ml-workloads.md)
- [133 - GPU调度与管理 (GPU Scheduling & Management)](./domain-11-ai-infra/03-gpu-scheduling-management.md)
- [GPU监控与可观测性](./domain-11-ai-infra/04-gpu-monitoring-dcgm.md)
- [分布式训练框架](./domain-11-ai-infra/05-distributed-training-frameworks.md)
- [AI数据处理Pipeline与特征工程](./domain-11-ai-infra/06-ai-data-pipeline.md)
- [AI实验管理与MLOps平台](./domain-11-ai-infra/07-ai-experiment-management.md)
- [AutoML与超参数调优](./domain-11-ai-infra/08-automl-hyperparameter-tuning.md)
- [AI模型注册中心与版本管理](./domain-11-ai-infra/09-model-registry.md)
- [AI模型部署与生命周期管理](./domain-11-ai-infra/10-model-deployment-management.md)
- [AI安全与模型保护](./domain-11-ai-infra/11-ai-security-model-protection.md)
- [Kubeflow AI 平台部署与实践指南](./domain-11-ai-infra/99-kubeflow-ai-platform-guide.md)

### LLM 与推理

- [144 - LLM推理服务部署](./domain-11-ai-infra/17-llm-inference-serving.md)
- [LLM模型Serving架构与推理优化](./domain-11-ai-infra/18-llm-serving-architecture.md)
- [146 - LLM模型量化技术](./domain-11-ai-infra/19-llm-quantization.md)
- [147 - 向量数据库与RAG架构](./domain-11-ai-infra/20-vector-database-rag.md)
- [LLM 成本监控与 FinOps](./domain-11-ai-infra/23-llm-cost-monitoring.md)
- [LLM 可观测性](./topic-dictionary/observability/llm-observability.md)

### 术语词典

- [08 - AI/ML基础设施专业词典](./topic-dictionary/specialized-workloads/ai-infra-specialist.md)
- [GPU 资源管理与分区技术](./topic-dictionary/specialized-workloads/gpu-resource-management-and-partitioning.md)
- [KServe 模型服务平台](./topic-dictionary/specialized-workloads/kserve-model-serving.md)
- [Kueue 作业队列与准入控制](./topic-dictionary/specialized-workloads/kueue-job-queue-management.md)
- [大语言模型（LLM）推理优化](./topic-dictionary/specialized-workloads/llm-inference-optimization.md)
- [MLOps 流水线与模型仓库](./topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry.md)
- [向量数据库与 RAG 基础设施](./topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure.md)
- [设备插件](./topic-dictionary/platform-engineering/device-plugins.md)
- [Dynamic Resource Allocation](./topic-dictionary/scheduling/dynamic-resource-allocation.md)
- [Gang Scheduling](./topic-dictionary/scheduling/gang-scheduling.md)

## 关联文档 (K8s 集成)

### 故障排查

- [AI/ML 工作负载故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting.md)
- [Kubeflow 平台故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md)
- [MPI Operator 与分布式训练故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting.md)
- [GPU 与设备插件故障排查指南](./topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting.md)

### 调度与资源

- [HPA 与 VPA 自动扩缩容故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting.md)
- [Cluster Autoscaler 节点自动扩缩容故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md)

### 成本与可观测性

- [141 - AI成本分析与FinOps实践 (AI Cost Analysis & FinOps)](./domain-11-ai-infra/12-ai-cost-analysis-finops.md)
- [AI平台可观测性体系](./domain-11-ai-infra/13-ai-platform-observability.md)
- [FinOps 成本优化与云费用故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting.md)

## 扩展参考

### AI 生态项目

- [KServe](./domain-34-cncf-landscape/incubating/kserve/kserve.md)
- [Kubeflow](./domain-34-cncf-landscape/incubating/kubeflow/kubeflow.md)
- [Volcano](./domain-34-cncf-landscape/incubating/volcano/volcano.md)
- [Fluid](./domain-34-cncf-landscape/incubating/fluid/fluid.md)
- [HAMi (Heterogeneous AI Computing Virtualization Middleware)](./domain-34-cncf-landscape/sandbox/hami/hami.md)
- [KAITO (Kubernetes AI Toolchain Operator)](./domain-34-cncf-landscape/sandbox/kaito/kaito.md)
- [HolmesGPT](./domain-34-cncf-landscape/sandbox/holmesgpt/holmesgpt.md)
- [ModelPack](./domain-34-cncf-landscape/sandbox/modelpack/modelpack.md)

### 技术论文

- [Kubernetes AI/ML GPU调度与LLM推理服务 (AI/ML GPU Scheduling and LLM Inference Serving)](./domain-19-papers/17-kubernetes-aiml-gpu-scheduling-llm-inference.md)
