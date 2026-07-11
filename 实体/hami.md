---
title: HAMI
description: '## 概述'
summary: 'HAMi（原 vGPU_4k8s）是一个异构计算设备虚拟化中间件，为 Kubernetes 提供 GPU、NPU 等加速器的共享和虚拟化能力。它允许多个 Pod 共享同一块物理 GPU，并提供显存和算力的精细化隔离，有效提升 GPU 利用率。HAMi 支持 NVIDIA GPU、AMD GPU、华为 Ascend NPU、寒武纪 MLU 等多种异构设备。'
category: entities
tags:
- k8s
- cncf
- observability
- hami
- scheduler
- prometheus
- grafana
- crd
- operator
- gpu
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HAMI 是什么
- 如何 HAMI
trigger_keywords:
- HAMI
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HAMI

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go, C

## 概述

HAMi（原 vGPU_4k8s）是一个异构计算设备虚拟化中间件，为 Kubernetes 提供 GPU、NPU 等加速器的共享和虚拟化能力。它允许多个 Pod 共享同一块物理 GPU，并提供显存和算力的精细化隔离，有效提升 GPU 利用率。HAMi 支持 NVIDIA GPU、AMD GPU、华为 Ascend NPU、寒武纪 MLU 等多种异构设备，在 AI/ML 工作负载场景中显著降低 GPU 硬件成本。

## 核心特性

- **GPU 共享**: 多个 Pod 共享同一块物理 GPU，支持显存隔离和算力隔离
- **多设备支持**: NVIDIA、AMD、华为 Ascend NPU、寒武纪 MLU
- **硬隔离**: 基于 GPU 硬件虚拟化技术（vGPU/MIG）实现显存和算力隔离
- **调度扩展**: 扩展 Kubernetes 调度器，支持 GPU 细粒度资源调度
- **监控集成**: 导出 GPU 使用率、显存占用等 Prometheus 指标
- **设备分片**: 支持 deviceSplitCount 控制每张 GPU 的最大共享数

## 架构

HAMi 通过 device plugin 和调度器扩展实现 GPU 虚拟化。核心组件包括：HAMi Scheduler（扩展 Kubernetes 调度器，感知 GPU 细粒度资源）、HAMi Device Plugin（替代原生 NVIDIA device plugin，上报虚拟化后的 GPU 资源）、HAMi Webhook（拦截 Pod 创建请求，注入 GPU 虚拟化配置）。设备层通过劫持 CUDA 调用或利用硬件虚拟化能力（如 NVIDIA MIG、vGPU）实现资源隔离。调度器维护全局 GPU 资源视图，根据 Pod 的 GPU 请求进行细粒度分配。

## Kubernetes 集成

HAMi 通过 Kubernetes Device Plugin Framework 上报虚拟化后的 GPU 资源（如 `nvidia.com/gpu.mem`、`nvidia.com/gpu.cores`），扩展调度器（Scheduler Extender 或 KubeSchedulerPlugin）实现细粒度 GPU 调度。Mutating Webhook 自动为 Pod 注入 GPU 虚拟化运行时配置。用户通过在 Pod spec 中添加 GPU 资源请求（如 `nvidia.com/gpu: 1` + `nvidia.com/gpu.mem: 2000`）来申请共享 GPU 资源。

## 生产使用场景

1. **推理服务共享 GPU**: 多个推理 Pod 共享一张 GPU，提升利用率降低成本
2. **开发/测试环境**: 多个开发者共享 GPU 资源进行模型调试
3. **多租户 GPU 隔离**: 不同团队的 GPU 任务实现资源隔离
4. **混合调度**: 推理任务使用 GPU 共享，训练任务使用 GPU 独占

## 安装

```bash
# Helm 安装
helm repo add hami https://project-hami.github.io/HAMi
helm install hami hami/hami --set devicePlugin.deviceMemoryScaling=2 \
  --set scheduler.kubeScheduler.imageTag=v0.28.9
# 使用 GPU 共享
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    resources:
      limits:
        nvidia.com/gpu: 1
        nvidia.com/gpumem: 2000
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **HAMi** | 多设备支持、显存隔离 | 非官方方案、稳定性待验证 |
| NVIDIA GPU Operator | 官方支持、MIG 集成 | 仅 NVIDIA、功能较基础 |
| Volcano GPU 调度 | 批处理调度、队列管理 | 无显存虚拟化 |
| Run:AI | 企业级 GPU 管理 | 商业产品 |

## 架构定位

在 CNCF 生态中，HAMi 属于 **Scheduling / AI Infrastructure** 类别，解决了 Kubernetes GPU 资源粒度过粗的问题。它与 Volcano、Kueue 等批处理调度器互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/container-runtime-comparison.md|container-runtime-comparison]]
- [[pod-lifecycle]]
- [[实体/kube-scheduler.md|kube-scheduler]]

## Related

- [[fluentd]] — Fluentd
- [[cubefs]] — CubeFS
- [[artifact-hub]] — Artifact Hub
- [[pipecd]] — PipeCD
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hami
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
