---
title: Kubeflow [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- ai-ml
- kubeflow
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubeflow 是什么
- 如何 Kubeflow
trigger_keywords:
- Kubeflow
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kubeflow

> **CNCF 状态**: Incubating | **类别**: AI/ML | **主要语言**: Python, Go

## 概述

Kubeflow 是一个 CNCF 孵化项目，最初由 Google 基于 TensorFlow 扩展内部 ML 基础设施开源，现由 Kubeflow 社区联合多家企业（Google、Cisco、IBM、AWS、Microsoft 等）共同维护。它是 Kubernetes 原生的机器学习平台，提供从数据准备、模型训练、超参数调优到模型服务的端到端 MLOps 能力。Kubeflow 将 ML 工作流的各个环节映射为 K8s 资源，让数据科学家能够在云原生环境中使用熟悉的工具（Jupyter、PyTorch、TensorFlow）进行 ML 开发。

## Key Features（核心能力）

- **Jupyter Notebook 服务**：在 K8s 上提供多用户 Jupyter Notebook 即服务
- **分布式训练**：支持 TensorFlow Training (TFJob) 和 PyTorch Training (PyTorchJob) CRD
- **Katib 超参数调优**：自动化的超参数搜索和神经架构搜索
- **KServe 模型服务**：将训练好的模型部署为可弹性伸缩的推理服务
- **Pipelines**：基于 Argo Workflow 的 ML 流水线编排，支持 DAG 依赖
- **多租户支持**：通过 Dex/Istio 实现用户认证和资源隔离

## 架构与工作原理

Kubeflow 由多个松耦合的组件构成：Notebook Controller 管理 Jupyter Notebook Pod；Training Operator 管理分布式训练任务（TFJob、PyTorchJob、MPIJob）；Katib Controller 管理超参数调优实验；KFP（Kubeflow Pipelines）基于 Argo Workflow 实现 ML 流水线；KServe 提供模型推理服务。所有组件通过 Istio Service Mesh 互联，通过 Dex/OIDC 提供认证授权。

## K8s 集成

Kubeflow 完全基于 Kubernetes 原生能力构建：训练任务通过 CRD 定义，由自定义 Controller 调度；通过 Volcano 或 K8s 默认调度器进行 GPU 资源调度；使用 PVC 管理训练数据；通过 Istio Service Mesh 管理组件间通信。安装通常通过 Kubeflow Manifests 或 Operator 进行，依赖 K8s 1.25+。

## 生产用例

- **分布式模型训练**：在 GPU 集群上运行大规模 TensorFlow/PyTorch 分布式训练
- **超参数调优**：使用 Katib 自动搜索最优模型参数
- **ML CI/CD 流水线**：通过 Kubeflow Pipelines 自动化数据准备到模型部署全流程
- **Jupyter 即服务**：为数据科学团队提供按需的 Notebook 环境

## 安装与快速开始

```bash
# 使用 Kubeflow manifests
VERSION=v1.9.0
git clone --branch ${VERSION} https://github.com/kubeflow/manifests.git
while ! kustomize build manifests/example | kubectl apply -f -; do echo "Retrying..."; sleep 10; done
```

## 对比替代方案

相比 MLflow（专注于实验跟踪和模型注册），Kubeflow 提供更完整的 ML 平台能力（训练、调优、服务、流水线）。相比 SageMaker/Azure ML 等云托管服务，Kubeflow 是开源的、可自托管的。

## Related

- [[kubean]] — Kubean
- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-kubeflow-ai-platform-guide
- troubleshooting.md|02-kubeflow-troubleshooting]]
- kubeflow
- [[实体/kaito.md|[[KAITO (Kubernetes AI Toolchain Operator)|KAITO]]]]
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
