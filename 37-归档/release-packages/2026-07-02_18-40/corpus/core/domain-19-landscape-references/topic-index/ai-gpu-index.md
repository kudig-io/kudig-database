---
title: AI / GPU 基础设施知识图谱索引
description: '## AI / GPU 知识图谱'
summary: '## AI / GPU 知识图谱'
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI / GPU 基础设施知识图谱索引

> 知识图谱：按关键字 **ai-gpu** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### AI 基础设施

- AI基础设施架构
- 132 - AI/ML工作负载运维 (AI/ML Workloads Operations)
- 133 - GPU调度与管理 (GPU Scheduling & Management)
- GPU监控与可观测性
- 分布式训练框架
- AI数据处理Pipeline与特征工程
- AI实验管理与MLOps平台
- AutoML与超参数调优
- AI模型注册中心与版本管理
- AI模型部署与生命周期管理
- AI安全与模型保护
- Kubeflow AI 平台部署与实践指南

### LLM 与推理

- 144 - LLM推理服务部署
- LLM模型Serving架构与推理优化
- 146 - LLM模型量化技术
- 147 - 向量数据库与RAG架构
- LLM 成本监控与 FinOps
- [[domain-17-system-foundation/知识字典/observability/llm-observability.md|LLM 可观测性]]

### 术语词典

- [[domain-17-system-foundation/知识字典/specialized-workloads/ai-infra-specialist.md|08 - AI/ML基础设施专业词典]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/gpu-resource-management-and-partitioning.md|GPU 资源管理与分区技术]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/kserve-model-serving.md|KServe 模型服务平台]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/kueue-job-queue-management.md|Kueue 作业队列与准入控制]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/llm-inference-optimization.md|大语言模型（LLM）推理优化]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/mlops-pipelines-and-model-registry.md|MLOps 流水线与模型仓库]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/vector-databases-and-rag-infrastructure.md|向量数据库与 RAG 基础设施]]
- [[domain-17-system-foundation/知识字典/platform-engineering/device-plugins.md|设备插件]]
- [[domain-17-system-foundation/知识字典/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]]
- [[domain-17-system-foundation/知识字典/scheduling/gang-scheduling.md|Gang Scheduling]]

## 关联文档 (K8s 集成)

### 故障排查

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|AI/ML 工作负载故障排查指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/02-kubeflow-troubleshooting|Kubeflow 平台故障排查指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/03-mpi-operator-troubleshooting|MPI Operator 与分布式训练故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]

### 调度与资源

- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|Cluster Autoscaler 节点自动扩缩容故障排查指南]]

### 成本与可观测性

- 141 - AI成本分析与FinOps实践 (AI Cost Analysis & FinOps)
- AI平台可观测性体系
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/04-finops-cost-optimization-troubleshooting|FinOps 成本优化与云费用故障排查指南]]

## 扩展参考

### AI 生态项目

- KServe
- Kubeflow
- Volcano
- Fluid
- HAMi (Heterogeneous AI Computing Virtualization Middleware)
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-15-specialized-tech/01-edge-computing/01-kubernetes-developer-toolchain-guide|99 kubernetes developer toolchain guide]]
- HolmesGPT
- ModelPack

### 技术论文

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-19-landscape-references/02-papers/06-kubernetes-aiml-gpu-scheduling-llm-inference|17 kubernetes aiml gpu scheduling llm inference]]


<!-- risk-assessed -->
