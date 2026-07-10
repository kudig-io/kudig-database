---
title: InterLink (entities)
description: '## 概述'
summary: 'InterLink 是一个 Virtual Kubeletet|Kubelet]] 提供者实现，允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。'
category: entities
tags:
- k8s
- cncf
- edge
- interlink
- kubelet
- prometheus
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
- InterLink 是什么
- 如何 InterLink
trigger_keywords:
- InterLink
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# InterLink

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Go

## 概述

InterLink 是一个 Virtual Kubeletet|Kubelet]] 提供者实现，允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。它通过标准的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 将传统 HPC 集群（Slurm、HTCondor）和云计算平台作为 Kubernetes 的扩展计算资源，使科研人员和工程师能够使用熟悉的 Kubernetes 工作流提交和管理 H...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **资源映射**: 合理配置虚拟节点容量，反映 HPC 集群的实际可用资源
- **数据预置**: 对于大型数据集，预先将数据放置到 HPC 共享文件系统，避免运行时传输
- **容器镜像**: 使用 Singularity/Apptainer 兼容的容器镜像，确保 HPC 环境兼容性
- **超时设置**: 根据 HPC 队列等待时间调整 Pod 超时阈值
- **监控**: 配置对虚拟节点状态的监控，及时发现 HPC 集群连接异常

## 架构定位

在 CNCF 生态中，interlink 属于 **Edge** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]
- [[实体/kubelet.md|kubelet]]

## Related

- [[实体/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[virtual-kubelet]] — Virtual Kubelet

- interlink
- [[实体/akri.md|Akri]]
- [[实体/openyurt.md|OpenYurt]]
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
