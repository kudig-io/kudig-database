---
title: Virtual Kubelet [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- virtual-kubelet
- kubelet
- scheduler
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Virtual Kubelet 是什么
- 如何 Virtual Kubelet
trigger_keywords:
- Virtual
- Kubelet
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Virtual [[kubelet|Kubelet]]

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

Virtual Kubelet 是一个开源框架，它模拟 Kubernetes kubelet，将自身注册为集群中的一个节点。但不同于真正的 kubelet 运行在物理/虚拟机上，Virtual Kubelet 将 Pod 调度到其他后端服务，如 Azure Container Instances (ACI)、AWS Fargate、HashiCorp Nomad 等无服务器容器平台。

## 核心能力

- **虚拟节点**: 在 K8s 中注册虚拟节点
- **多后端**: ACI、Fargate、Nomad、OpenStack
- **无限扩展**: 无需管理底层节点基础设施
- **标准 API**: 兼容 Kubernetes Pod API
- **弹性伸缩**: 实现真正的无服务器容器
- **Provider 接口**: 可扩展的 Provider 插件架构

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Taint/Toleration**: 使用 taint 控制调度到虚拟节点
- **资源限制**: 设置合理的资源请求
- **网络规划**: 注意虚拟节点的网络连通性
- **持久化**: 虚拟节点通常不支持本地存储

## 架构定位

在 CNCF 生态中，virtual-kubelet 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]
- [[pod-lifecycle]]
- [[entities/kubelet.md|kubelet]]
- [[entities/kube-scheduler.md|kube-scheduler]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[openfeature]] — OpenFeature
- tools]] — [[Podman Desktop|Podman Desktop]]
- [[k3s]] — k3s 轻量级 Kubernetes
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- virtual-kubelet
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
