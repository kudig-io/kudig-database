---
title: K8s 专用工作负载术语参考
description: 'description: ''\| **适合读者** \| ML工程师入门K8s → 平台工程师管理AI工作... |'
category: references
tags:
- k8s
- dictionary
- specialized-workloads
- etcd
- scheduler
- prometheus
- grafana
- istio
- opa
- ceph
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 专用工作负载术语参考 是什么
- 如何 K8s 专用工作负载术语参考
trigger_keywords:
- K8s
- 专用工作负载术语参考
prerequisites:
- kubectl-basics
- pod-lifecycle
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- gpu-scheduling-basics
- policy-basics
created: "2026-05-23"
---

# K8s 专用工作负载术语参考

本页汇总了 **专用工作负载** 领域的 10 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[references/k8s-ai-infrastructure|k8s-ai-infrastructure]] | [[references/k8s-workload-management|k8s-workload-management]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **08 - AI/ML基础设施专业词典** | Ai Infra Specialist | title: 08 - AI/ML基础设施专业词典
description: '\| **适合读者** \| ML工程师入门K8s → 平台工程师管理AI工作... |
| **GPU 资源管理与分区技术** | Gpu Resource Management And Partitioning | 在 Kubernetes 上运行 AI/ML 工作负载时，GPU 是最昂贵且最稀缺的资源 |
| **在 Kubernetes 中运行 Windows 容器指南** | Guide For Running Windows Containers In Kubernetes | 本指南提供了在 Kubernetes 集群中运行 Windows 容器的实操步骤和注意事项 |
| **高性能计算与生物信息学（HPC & Bioinformatics）** | Hpc And Bioinformatics | **高性能计算（HPC, High-Performance Computing）** 和 **生物信息学（Bioinformatics）** 是计算密集型... |
| **KServe 模型服务平台** | Kserve Model Serving | **KServe** 是 Kubernetes 上领先的**云原生模型推理服务平台**，提供标准化的模型部署、自动扩缩容（包括缩至零）、金丝雀发布、A/B... |
| **Kueue 作业队列与准入控制** | Kueue Job Queue Management | **Kueue** 是 Kubernetes 官方推出的**作业队列与集群级资源配额管理系统**，专门解决 AI/ML、批处理（Batch）和高性能计算（... |
| **大语言模型（LLM）推理优化** | Llm Inference Optimization | 随着大语言模型（LLM）在生产环境的广泛部署，推理成本已成为 AI 基础设施的最大开支项 |
| **MLOps 流水线与模型仓库** | Mlops Pipelines And Model Registry | **MLOps（Machine Learning Operations）** 是将 DevOps 工程实践应用于机器学习生命周期的方法论 |
| **向量数据库与 RAG 基础设施** | Vector Databases And Rag Infrastructure | **RAG（Retrieval-Augmented Generation，检索增强生成）** 是 2025–2026 年企业级 LLM 应用的核心架构模式 |
| **Windows 容器在 Kubernetes 中的支持** | Windows Containers In Kubernetes | Windows 应用程序在众多组织的服务和应用中占有很大比例 |

---

### 08 - AI/ML基础设施专业词典

title: 08 - AI/ML基础设施专业词典
description: '| **适合读者** | ML工程师入门K8s → 平台工程师管理AI工作负载 → SRE优化AI基础设施 |'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- scheduler
- prometheus
- grafana
- istio
- opa
- ceph
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- AI/ML基础设施专业词典 是什么
- 如何 AI/ML基础设施专业词典
trigger_keywords:
- AI
- ML基础设施专业词典
- dictionary
title_en: Ai Infra Specialist
authors:
- name: KUDIG Team
  rol...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/ai-infra-specialist.md`）*

---

### GPU 资源管理与分区技术

在 Kubernetes 上运行 AI/ML 工作负载时，GPU 是最昂贵且最稀缺的资源。2026 年的行业最佳实践要求平台团队不仅要将 GPU 暴露给 Pod，还需通过**分区（Partitioning）、共享（Sharing）、拓扑感知调度（Topology-Aware Scheduling）**等手段，将 GPU 利用率从传统的 13%–40% 提升至 70% 以上，从而显著降低 AI 基础设施成本。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/gpu-resource-management-and-partitioning.md`）*

---

### 在 Kubernetes 中运行 Windows 容器指南

本指南提供了在 Kubernetes 集群中运行 Windows 容器的实操步骤和注意事项。在 Kubernetes 上创建和部署服务与工作负载时，Windows 容器与 Linux 容器的行为大体相同，`kubectl` 命令也完全一致。本文通过示例帮助用户快速上手 Windows 容器的部署、调度、可观测性和身份管理。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/guide-for-running-windows-containers-in-kubernetes.md`）*

---

### 高性能计算与生物信息学（HPC & Bioinformatics）

**高性能计算（HPC, High-Performance Computing）** 和 **生物信息学（Bioinformatics）** 是计算密集型工作负载的典型代表。随着基因组测序、蛋白质结构预测（AlphaFold）、药物分子模拟等应用的爆炸式增长，传统 HPC 中心（基于 Slurm、PBS）正在与 Kubernetes 融合。2026 年的最佳实践表明，通过 **Volcano、Kueue、MPI Operator** 等工具，Kubernetes 已能够有效管理 HPC 作业调度、大规模并行计算和 GPU 集群资源，为科学研究提供云原生的弹性算力平台。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/hpc-and-bioinformatics.md`）*

---

### KServe 模型服务平台

**KServe** 是 Kubernetes 上领先的**云原生模型推理服务平台**，提供标准化的模型部署、自动扩缩容（包括缩至零）、金丝雀发布、A/B 测试以及多框架支持。作为 CNCF 孵化的项目，KServe 在 2025–2026 年已成为企业级 AI 推理基础设施的事实标准。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve-model-serving.md`）*

---

### Kueue 作业队列与准入控制

**Kueue** 是 Kubernetes 官方推出的**作业队列与集群级资源配额管理系统**，专门解决 AI/ML、批处理（Batch）和高性能计算（HPC）场景下的资源争抢与调度公平性问题。在 2026 年的 AI 基础设施实践中，Kueue 已成为管理 GPU 集群稀缺资源的标配工具。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/kueue-job-queue-management.md`）*

---

### 大语言模型（LLM）推理优化

随着大语言模型（LLM）在生产环境的广泛部署，推理成本已成为 AI 基础设施的最大开支项。2026 年的最佳实践表明，通过 **Continuous Batching（连续批处理）、Quantization（量化）、Parallelism（并行策略）** 以及 **Prefill/Decode 分离** 等优化手段，可以将 LLM 推理的 GPU 利用率从约 40% 提升至 90% 以上，并将单 token 成本降低 85%。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/llm-inference-optimization.md`）*

---

### MLOps 流水线与模型仓库

**MLOps（Machine Learning Operations）** 是将 DevOps 工程实践应用于机器学习生命周期的方法论。2026 年的行业最佳实践要求 AI 基础设施具备完整的**数据准备、模型训练、实验追踪、模型注册、自动部署与监控反馈**能力。在 Kubernetes 上，MLOps 通常通过 **Kubeflow、MLflow、Airflow** 等工具链以及原生的 Jobs/CronJobs/Pipelines 来实现。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry.md`）*

---

### 向量数据库与 RAG 基础设施

**RAG（Retrieval-Augmented Generation，检索增强生成）** 是 2025–2026 年企业级 LLM 应用的核心架构模式。RAG 通过将用户查询与私有知识库中的相关文档片段进行语义匹配，再将检索结果注入 LLM Prompt，从而显著提升回答的准确性、时效性和可溯源性。支撑 RAG 的底层基础设施是**向量数据库（Vector Database）** 和 **Embedding Pipeline**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure.md`）*

---

### Windows 容器在 Kubernetes 中的支持

Windows 应用程序在众多组织的服务和应用中占有很大比例。Windows 容器提供了一种封装进程和打包依赖的方式，使得 Windows 应用也能采用 DevOps 实践并遵循云原生模式。通过在现有的 Linux 集群中加入 Windows 节点，组织无需为不同操作系统寻找单独的编排器，从而提升整体运维效率。

Kubernetes 支持在 Windows 节点上运行 Windows 容器（仅支持进程隔离模式，不支持 Hyper-V 隔离模式）。控制平面必须运行在 Linux 上，而工作节点可以是 Windows 或 Linux。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/specialized-workloads/windows-containers-in-kubernetes.md`）*

---

## 相关页面

- [[references/k8s-ai-infrastructure|k8s-ai-infrastructure]]
- [[references/k8s-workload-management|k8s-workload-management]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/specialized-workloads/ai-infra-specialist.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/gpu-resource-management-and-partitioning.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/guide-for-running-windows-containers-in-kubernetes.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/hpc-and-bioinformatics.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve-model-serving.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/kueue-job-queue-management.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/llm-inference-optimization.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure.md`
- `domain-17-system-foundation/topic-dictionary/specialized-workloads/windows-containers-in-kubernetes.md`

## Related

- [[kserve]] — KServe
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management|resource-management]] — Resource Management (Requests, Limits, QoS)
