---
title: Cohdi
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- cohdi
- crd
- operator
- gpu
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cohdi 是什么
- 如何 Cohdi
trigger_keywords:
- Cohdi
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Cohdi

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

CoHDI（Composable Hyperconverged Disaggregated Infrastructure）是一个 Kubernetes Operator，用于在分解式基础设施中动态组合和管理硬件资源。它支持通过 CXL（Compute Express Link）和 PCIe 总线动态地将远端 GPU、内存、存储等设备组合分配给 Kubernetes Pod，使得计算节点可以按...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **硬件规划**: 确保网络结构支持 CXL/PCIe Fabric，合理规划交换机拓扑
- **资源池分级**: 按性能等级划分资源池，高优先级任务使用高性能设备
- **亲和性策略**: 为延迟敏感任务配置拓扑亲和性，减少跨交换机访问
- **容量规划**: 监控资源池利用率，及时扩展物理设备
- **故障隔离**: 配置设备健康检查，自动隔离问题设备

## 架构定位

在 CNCF 生态中，cohdi 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[devfile]] — Devfile
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cohdi
- index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
