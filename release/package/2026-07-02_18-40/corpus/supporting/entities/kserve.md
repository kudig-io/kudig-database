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
last_updated: 2026-05
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

KServe（前身 KFServing）是 Kubernetes 上的标准化模型推理平台。它提供无服务器推理、自动扩缩容、金丝雀部署和模型解释能力，支持 TensorFlow、PyTorch、scikit-learn、XGBoost 等主流框架。

## 核心能力

- **标准化接口**: 统一的 V1/V2 推理协议
- **多框架支持**: TensorFlow、PyTorch、Triton、ONNX、XGBoost 等
- **Serverless**: 基于 Knative 的自动扩缩容（可缩至零）
- **高级部署**: 金丝雀发布、A/B 测试、蓝绿部署
- **模型解释**: 集成 Alibi Explainer 提供可解释性
- **GPU 支持**: 自动 GPU 调度和资源管理

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模型版本管理**: 使用 storageUri 路径区分版本
- **资源配置**: 根据模型大小配置合适的 memory/GPU
- **健康检查**: 配置 liveness/readiness probe
- **预热**: 生产环境设置 min-scale >= 1 避免冷启动
- **监控告警**: 监控推理延迟和错误率

## 架构定位

在 CNCF 生态中，kserve 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[07-containerd-disaster-recovery]] — containerd 灾难恢复
- [[chaosblade]] — ChaosBlade
- [[network-service-mesh]] — Network Service Mesh (NSM)
- [[knative]] — Knative
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kserve
- [[entities/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[entities/root-terms.md|K8s Root术语参考]] — Cross-reference
- [[skills/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[entities/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
