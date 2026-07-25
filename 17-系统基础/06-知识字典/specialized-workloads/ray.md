---
title: Ray
description: Ray 是一个通用的分布式计算框架，擅长大规模 AI/ML 工作负载。通过 KubeRay Operator 部署到 Kubernetes
  中，提供弹性 GPU...
summary: Ray 是一个通用的分布式计算框架，擅长大规模 AI/ML 工作负载。通过 KubeRay Operator 部署到 Kubernetes 中，提供弹性
  GPU...
category: dictionary
tags:
- k8s
- glossary
- ray
- distributed-computing
- ai
- ml
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ray 是什么
- Ray 详解
trigger_keywords:
- Ray
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Ray

> **英文名**: Ray

## 概述

Ray 是一个通用的分布式计算框架，擅长大规模 AI/ML 工作负载。通过 KubeRay Operator 部署到 Kubernetes 中，提供弹性 GPU 集群、分布式训练和模型服务能力，已成为 AI 基础设施的事实标准之一。

## 核心概念/原理

### 核心架构

| 组件 | 功能 |
|------|------|
| Ray Head | 集群管理、调度、GCS（Global Control Store） |
| Ray Worker | 执行分布式任务的计算节点 |
| Ray Dashboard | Web UI 监控和调试 |
| KubeRay Operator | K8s 原生部署和管理 |

### Ray 生态

- **Ray Train**：分布式训练（PyTorch、TensorFlow、HuggingFace）。
- **Ray Tune**：超参调优和实验管理。
- **Ray Serve**：在线模型推理和组合。
- **Ray Data**：大规模数据处理。

## 关键机制或特性

- **弹性伸缩**：RayCluster 根据负载自动扩缩 Worker 节点。
- **GPU 调度**：支持 GPU 亲和性和共享（fractional GPU）。
- **Ray Job**：一次性提交和运行分布式任务。
- **Fault Tolerance**：Worker 故障自动恢复。
- 与 Kubernetes 生态集成（Ingress、RBAC、ResourceQuota）。

## 使用场景与最佳实践

- 大规模 AI 训练使用 Ray Train 替代单机训练。
- 使用 Ray Serve 部署 ML 模型的在线推理服务。
- 配合 KubeRay Operator 实现 Ray 集群的 K8s 原生管理。
- 使用 Ray Autoscaler 实现按需 GPU 资源伸缩。
- 通过 Ray Dashboard 监控任务执行和资源使用。

## 参考链接

- [Ray Official](https://docs.ray.io/)

## Related

- [[17-系统基础/06-知识字典/specialized-workloads/kubeflow.md|Kubeflow]]
- [[17-系统基础/06-知识字典/specialized-workloads/kserve.md|KServe]]
- [[17-系统基础/06-知识字典/workloads/job.md|Job]]
- [[17-系统基础/06-知识字典/scheduling/hpa.md|HPA]]
- [[17-系统基础/06-知识字典/platform-engineering/operator-pattern.md|Operator Pattern]]


<!-- risk-assessed -->
