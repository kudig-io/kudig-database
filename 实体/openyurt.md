---
title: OpenYurt (entities)
description: '## 概述'
summary: 'OpenYurt 是阿里云开源的边缘计算平台，将原生 Kubernetes 能力无缝扩展到边缘场景。它解决了边缘网络不稳定、节点自治、多区域管理等边缘计算特有挑战。'
category: entities
tags:
- k8s
- cncf
- edge
- openyurt
- kubelet
- cilium
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenYurt 是什么
- 如何 OpenYurt
trigger_keywords:
- OpenYurt
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenYurt

> **CNCF 状态**: Incubating | **类别**: Edge | **主要语言**: Go

## 概述

OpenYurt 是阿里云开源的边缘计算平台，将原生 Kubernetes 能力无缝扩展到边缘场景。它解决了边缘网络不稳定、节点自治、多区域管理等边缘计算特有挑战。

## 核心能力

- **边缘自治**: 边缘节点与云端断连时仍可正常工作
- **单元化管理**: NodePool 实现边缘节点分组管理
- **流量闭环**: 确保应用流量在同一 NodePool 内闭环
- **云边协同**: 统一的云端控制面管理边缘节点
- **无侵入增强**: 无需修改 Kubernetes 核心组件
- **边缘设备管理**: 集成 EdgeX Foundry 管理 IoT 设备

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **NodePool 规划**: 按地理位置、业务单元划分 NodePool
- **本地缓存**: 配置合适的 Yurt-Hub 缓存策略
- **流量闭环**: 为边缘服务配置拓扑感知
- **弹性部署**: 使用 YurtAppSet 管理跨 NodePool 应用
- **网络规划**: 确保云边隧道的网络可达性

## 架构定位

在 CNCF 生态中，openyurt 属于 **Edge** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- networking.md|cilium-ebpf-networking]]
- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[实体/kubelet.md|[[kubelet|kubelet]]]]

## Related

- [[paralus]] — Paralus
- [[hexa]] — Hexa
- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-openyurt-architecture
- openyurt
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
