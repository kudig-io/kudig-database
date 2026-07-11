---
title: KServe (entities)
description: '## 概述'
summary: 'KServe（前身 KFServing）是 Kubernetes 上的标准化模型推理平台。它提供无服务器推理、自动扩缩容、金丝雀部署和模型解释能力，支持 TensorFlow、PyTorch、scikit-learn、XGBoost 等主流框架。'
category: entities
tags:
- k8s
- cncf
- observability
- kserve
- prometheus
- grafana
- istio
- containerd
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KServe 是什么
- 如何 KServe
trigger_keywords:
- KServe
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KServe

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Python, Go

## 概述

KServe（前身 KFServing）是 Kubeflow 孵化、现独立运营的 Kubernetes 标准化模型推理平台，2022 年加入 CNCF Incubating。它提供无服务器推理（Serverless Inference）、自动扩缩容、金丝雀部署和模型解释能力，支持 TensorFlow、PyTorch、scikit-learn、XGBoost、ONNX 等主流 ML 框架。KServe 通过统一的推理协议简化了从模型训练到生产部署的流程，是云原生 MLOps 的核心组件。

## 核心特性

- **标准化推理协议**: 统一的 V1（REST）和 V2（gRPC/REST）推理协议
- **多框架 InferenceRuntime**: 支持 PyTorch、TensorFlow、Triton、ONNX、XGBoost、HuggingFace
- **Serverless**: 基于 Knative 的自动扩缩容，支持缩容到零
- **高级部署**: 金丝雀发布、A/B 测试、蓝绿部署
- **模型解释**: 集成 Alibi Explainer 提供预测可解释性
- **GPU 支持**: 自动 GPU 调度、显存管理和多模型共享

## 架构

KServe 由 KServe Controller、InferenceService CRD 和 Model Agent 组成。Controller 监听 InferenceService CRD，创建 Knative Service（或原生 Deployment）。InferenceService 定义推理服务规格——Predictor（模型存储 URI、框架、资源）、Transformer（预处理/后处理）、Explainer（模型解释）。每个 Pod 包含 Model Agent（拉取模型权重）和 Model Server（运行推理）。基于 Knative 的 Activator 实现冷启动——流量到达时自动扩容从零到一。

## Kubernetes 集成

KServe 通过 InferenceService CRD 声明式管理推理服务。InferenceService 定义模型来源（storageUri）、运行时（框架）、资源配置（CPU/GPU/Memory）。Controller 创建 Knative Service 或原生 K8s Service + Deployment。基于 Knative KPA（Knative Pod Autoscaler）实现从零到 N 的自动扩缩容。与 Istio/Kourier 等 Ingress/Gateway 集成暴露推理端点。通过 KServe Model Agent 从 S3/GCS/HuggingFace 拉取模型。

## 生产使用场景

1. **模型生产部署**: 将训练好的 ML 模型部署为可伸缩的推理 API
2. **金丝雀发布**: 将 10% 流量导入新版本模型，验证效果后全量切换
3. **GPU 成本优化**: 使用 Scale-to-Zero 在无请求时释放 GPU 资源
4. **多模型部署**: 在同一 GPU 上部署多个模型共享计算资源

## 安装

```bash
# 安装 Knative（KServe 依赖）
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.14.0/serving-core.yaml
# 安装 KServe
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.12.1/kserve.yaml
# 部署推理服务
kubectl apply -f - <<EOF
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata: { name: iris-classifier }
spec:
  predictor:
    minReplicas: 1
    pytorch:
      storageUri: s3://models/iris
      resources:
        limits: { nvidia.com/gpu: 1 }
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **KServe** | CNCF Incubating、标准协议 | Knative 依赖较重 |
| Seldon Core | 功能丰富、企业级 | 架构复杂 |
| BentoML | 端到端 MLOps | 非 K8s 原生 |
| Triton Inference Server | NVIDIA 官方、高性能 | 仅 NVIDIA GPU |

## 架构定位

在 CNCF 生态中，KServe 属于 **AI/ML / Serverless** 类别，是云原生模型推理的标准化平台。它定义了推理服务的通用 API 标准。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[07-containerd-disaster-recovery]] — containerd 灾难恢复
- [[chaosblade]] — ChaosBlade
- [[network-service-mesh]] — Network Service Mesh (NSM)
- [[knative]] — Knative
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kserve
- [[实体/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[实体/root-terms.md|K8s Root术语参考]] — Cross-reference
- [[技能/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
