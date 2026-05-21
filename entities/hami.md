---
title: HAMI
description: '## 概述'
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
last_updated: 2026-05
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

# HAMI

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go, C

## 概述

HAMi（原 vGPU_4k8s）是一个异构计算设备虚拟化中间件，为 Kubernetes 提供 GPU、NPU 等加速器的共享和虚拟化能力。它允许多个 Pod 共享同一块物理 GPU，并提供显存和算力的精细化隔离，有效提升 GPU 利用率。HAMi 支持 NVIDIA GPU、AMD GPU、华为 Ascend NPU、寒武纪 MLU 等多种异构设备。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **资源规划**: 根据模型的实际显存和算力需求设置 limits，避免过度超分
- **监控告警**: 部署 GPU 监控面板，对显存使用率和 OOM 事件设置告警
- **拓扑感知**: 多 GPU 训练任务启用拓扑感知调度，优先分配 NVLink 连接的 GPU
- **分级策略**: 推理服务使用 GPU 共享提升利用率，训练任务使用独占模式保证性能
- **设备分片**: 根据业务需求合理设置 deviceSplitCount，避免过多碎片化

## 架构定位

在 CNCF 生态中，hami 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]]
- [[pod-lifecycle]]
- [[entities/kube-scheduler.md|kube-scheduler]]

## Related

- [[fluentd]] — Fluentd
- [[cubefs]] — CubeFS
- [[artifact-hub]] — Artifact Hub
- [[pipecd]] — PipeCD
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/hami/hami.md|hami]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
