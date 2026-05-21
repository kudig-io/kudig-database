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
- hpa
- vpa
- job
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
---

# AI / GPU 基础设施知识图谱索引

> 知识图谱：按关键字 **ai-gpu** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### AI 基础设施

- [[domain-14-ai-ml-infra/01-ai-infrastructure-overview|AI基础设施架构]]
- [[domain-14-ai-ml-infra/02-ai-ml-workloads|132 - AI/ML工作负载运维 (AI/ML Workloads Operations)]]
- [[domain-14-ai-ml-infra/03-gpu-scheduling-management|133 - GPU调度与管理 (GPU Scheduling & Management)]]
- [[domain-14-ai-ml-infra/04-gpu-monitoring-dcgm|GPU监控与可观测性]]
- [[domain-14-ai-ml-infra/05-distributed-training-frameworks|分布式训练框架]]
- [[domain-14-ai-ml-infra/06-ai-data-pipeline|AI数据处理Pipeline与特征工程]]
- [[domain-14-ai-ml-infra/07-ai-experiment-management|AI实验管理与MLOps平台]]
- [[domain-14-ai-ml-infra/08-automl-hyperparameter-tuning|AutoML与超参数调优]]
- [[domain-14-ai-ml-infra/09-model-registry|AI模型注册中心与版本管理]]
- [[domain-14-ai-ml-infra/10-model-deployment-management|AI模型部署与生命周期管理]]
- [[domain-14-ai-ml-infra/11-ai-security-model-protection|AI安全与模型保护]]
- [[domain-14-ai-ml-infra/99-kubeflow-ai-platform-guide|Kubeflow AI 平台部署与实践指南]]

### LLM 与推理

- [[domain-14-ai-ml-infra/17-llm-inference-serving|144 - LLM推理服务部署]]
- [[domain-14-ai-ml-infra/18-llm-serving-architecture|LLM模型Serving架构与推理优化]]
- [[domain-14-ai-ml-infra/19-llm-quantization|146 - LLM模型量化技术]]
- [[domain-14-ai-ml-infra/20-vector-database-rag|147 - 向量数据库与RAG架构]]
- [[domain-14-ai-ml-infra/23-llm-cost-monitoring|LLM 成本监控与 FinOps]]
- [[domain-17-system-foundation/topic-dictionary/observability/llm-observability|LLM 可观测性]]

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/ai-infra-specialist|08 - AI/ML基础设施专业词典]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/gpu-resource-management-and-partitioning|GPU 资源管理与分区技术]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve-model-serving|KServe 模型服务平台]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kueue-job-queue-management|Kueue 作业队列与准入控制]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/llm-inference-optimization|大语言模型（LLM）推理优化]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry|MLOps 流水线与模型仓库]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure|向量数据库与 RAG 基础设施]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/device-plugins|设备插件]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/dynamic-resource-allocation|Dynamic Resource Allocation]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/gang-scheduling|Gang Scheduling]]

## 关联文档 (K8s 集成)

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|AI/ML 工作负载故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting|Kubeflow 平台故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting|MPI Operator 与分布式训练故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting|GPU 与设备插件故障排查指南]]

### 调度与资源

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/03-cluster-autoscaler-troubleshooting|Cluster Autoscaler 节点自动扩缩容故障排查指南]]

### 成本与可观测性

- [[domain-14-ai-ml-infra/12-ai-cost-analysis-finops|141 - AI成本分析与FinOps实践 (AI Cost Analysis & FinOps)]]
- [[domain-14-ai-ml-infra/13-ai-platform-observability|AI平台可观测性体系]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting|FinOps 成本优化与云费用故障排查指南]]

## 扩展参考

### AI 生态项目

- [[domain-19-landscape-references/incubating/kserve/kserve|KServe]]
- [[domain-19-landscape-references/incubating/kubeflow/kubeflow|Kubeflow]]
- [[domain-19-landscape-references/incubating/volcano/volcano|Volcano]]
- [[domain-19-landscape-references/incubating/fluid/fluid|Fluid]]
- [[domain-19-landscape-references/sandbox/hami/hami|HAMi (Heterogeneous AI Computing Virtualization Middleware)]]
- [[domain-19-landscape-references/sandbox/kaito/kaito|KAITO (Kubernetes AI Toolchain Operator)]]
- [[domain-19-landscape-references/sandbox/holmesgpt/holmesgpt|HolmesGPT]]
- [[domain-19-landscape-references/sandbox/modelpack/modelpack|ModelPack]]

### 技术论文

- [[domain-19-landscape-references/17-kubernetes-aiml-gpu-scheduling-llm-inference|Kubernetes AI/ML GPU调度与LLM推理服务 (AI/ML GPU Scheduling and LLM Inference Serving)]]
