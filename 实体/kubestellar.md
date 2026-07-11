---
title: KubeStellar [entities]
description: '## 概述'
summary: 'KubeStellar 是一个多集群配置管理和工作负载分发平台，专注于将 Kubernetes 资源从中心控制面高效地分发到大量边缘集群。它采用 kcp（Kubernetes-like Control Plane）作为核心，支持管理数千个集群，特别适合边缘计算、零售、IoT 等需要管理大量分布式集群的场景。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubestellar
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
- KubeStellar 是什么
- 如何 KubeStellar
trigger_keywords:
- KubeStellar
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeStellar

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeStellar 是一个 CNCF 沙箱项目，由 IBM 和 Red Hat 联合推动，是一个 Kubernetes 多集群管理工具。与传统的 Hub-Spoke 模式不同，KubeStelar 采用独特的「multi-cluster flexible control plane」设计，允许在任何集群上运行控制平面，管理工作负载到任意数量目标集群的分发。它支持在不需要额外控制平面节点的情况下，实现跨集群的工作负载编排和配置同步。

## Key Features（核心能力）

- **灵活控制平面**：无需独立 Hub，任意 K8s 集群可作为控制平面
- **工作负载分发**：将 K8s 资源分发到多个目标集群
- **Binding 模式**：通过 Binding CRD 灵活定义资源到集群的映射
- **差异化配置**：通过 Transformation Pipeline 支持不同集群的定制化
- **WebSocket 隧道**：支持通过 WebSocket 穿透网络限制连接目标集群
- **边缘友好**：支持边缘集群的离线操作和增量同步

## 架构与工作原理

KubeStellar 引入了 Control Plane 和 Workload Space 分离的概念。在任意 K8s 集群上安装 KubeStellar 控制器后，该集群成为管理多个 Workload Space 的控制平面。通过 Placement CRD 定义工作负载分发策略，通过 Binding CRD 将资源绑定到目标集群。Transport Controller 通过 K8s API（或 WebSocket 隧道）将工作负载推送到各目标集群。

## K8s 集成

KubeStelar 在核心集群上通过 CRD 扩展 Kubernetes API：Placement CRD 定义分发策略（目标集群选择器、同步规则）；Binding CRD 定义资源与目标集群的映射。Workload Description Object 定义需要分发的工作负载资源。Transport Controller 在后台管理各目标集群的连接和工作负载同步，支持 K8s 原生 API 和 WebSocket 隧道两种传输方式。

## 生产用例

- **多集群应用分发**：将工作负载分发到大量边缘和数据中心集群
- **分层管理**：中心集群管理区域集群，区域集群管理边缘集群
- **混合云编排**：跨本地和公有云的工作负载管理
- **边缘 IoT**：大规模 IoT 边缘节点的应用分发

## 安装与快速开始

```bash
kubectl apply -f https://github.com/kubestellar/kubestellar/releases/latest/download/kubestellar.yaml
# 或使用 kubeflex
kubeflex init
kubeflex create controlplane ks-management
```

## 对比替代方案

相比 Karmada（需要独立控制平面），KubeStelar 更灵活——任意集群可作为控制平面。相比 KubeFleet，KubeStelar 更关注边缘场景和灵活拓扑。

## Related

- [[05-containerd-windows-support]] — [[containerd|containerd]]rd Windows 支持|containerd Windows 支持]]
- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kcp]] — kcp
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubestellar
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
