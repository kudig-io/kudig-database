---
title: Virtual Kubelet [entities]
description: '## 概述'
summary: 'Virtual Kubelet 是一个开源框架，它模拟 Kubernetes kubelet，将自身注册为集群中的一个节点。但不同于真正的 kubelet 运行在物理/虚拟机上，Virtual Kubelet 将 Pod 调度到其他后端服务，'
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
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Virtual [[kubelet|Kubelet]]

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

Virtual Kubelet 是由 Microsoft 开源的开源框架，2019 年加入 CNCF Sandbox。它模拟 Kubernetes kubelet，将自身注册为集群中的一个节点，但不同于真正的 kubelet 运行在物理/虚拟机上，Virtual Kubelet 将 Pod 调度到其他后端服务，如 Azure Container Instances（ACI）、AWS Fargate、HashiCorp Nomad 等无服务器容器平台。它使 Kubernetes 集群能够弹性扩展到云端无服务器基础设施，无需管理底层节点。

## 核心特性

- **虚拟节点**: 在 K8s 集群中注册为一个 Node，对调度器透明
- **多 Provider 后端**: 支持 ACI、Fargate、Nomad、ECS、OpenStack Zun 等
- **Provider 接口**: 定义标准接口，可自定义实现新的后端
- **弹性扩展**: 无需预置节点即可运行突发工作负载
- **标准 Pod API**: 兼容 Kubernetes Pod 生命周期管理
- **成本优化**: 仅在实际运行 Pod 时计费

## 架构

Virtual Kubelet 核心是一个实现 Kubernetes kubelet 接口的进程。它通过 Node API 将自身注册为集群节点，配置特定的 Taint 以避免普通 Pod 被调度到此节点。当 Pod 被调度到虚拟节点时，Virtual Kubelet 的 Provider 接口将 Pod 定义转换为后端服务（如 ACI）的容器实例并启动。Pod 状态通过 watch 机制持续同步回 Kubernetes。Provider 接口定义了 CreatePod、DeletePod、GetPod、GetPodStatus 等核心方法，每个 Provider 负责具体的后端对接。

## Kubernetes 集成

Virtual Kubelet 通过 Kubernetes Node API 注册为集群节点，具有特定的 Taint（`virtual-kubelet.io/provider`）。Pod 通过 Toleration 显式调度到虚拟节点。它实现了 Pod 生命周期管理（创建、删除、状态查询），但网络、存储和 Secret 等方面可能因 Provider 而异。Provider 负责将 Kubernetes Pod Spec 映射到底层平台的容器实例，包括环境变量、卷挂载、端口映射等配置。

## 生产使用场景

1. **突发流量处理**: 在流量高峰期将溢出的 Pod 调度到 ACI/Fargate，无需扩容节点
2. **CI/CD 作业**: 将构建任务调度到无服务器平台，节省集群资源
3. **混合调度**: 关键服务运行在自管节点，非关键任务运行在虚拟节点
4. **边缘扩展**: 在边缘集群中使用 Virtual Kubelet 连接云端无服务器后端

## 安装

```bash
# Azure ACI Provider
helm repo add virtual-kubelet https://virtual-kubelet.github.io
helm install virtual-kubelet virtual-kubelet/virtual-kubelet \
  --set provider=azure --set env.azureSubscriptionId=<id>
# 或使用 CLI
vkubelet --provider azure --nodeName virtual-node-aci
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Virtual Kubelet** | 标准化接口、多 Provider | 部分功能受限（存储/网络） |
| KEDA + Jobs | 事件驱动、原生 K8s | 仅支持缩容到零，不能扩展节点 |
| Karpenter | 自动节点供给、高性能 | 仅支持节点级扩展，非无服务器 |
| ACK (Alibaba) | 云厂商深度集成 | 厂商绑定 |

## 架构定位

在 CNCF 生态中，Virtual Kubelet 属于 **Runtime / Orchestration** 类别，是 Kubernetes 与无服务器计算之间的桥梁。它扩展了 Kubernetes 的弹性能力边界。

## 参考链接

- [[operator-pattern]]
- [[pod-lifecycle]]
- [[实体/kubelet.md|kubelet]]
- [[实体/kube-scheduler.md|kube-scheduler]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[openfeature]] — OpenFeature
- tools]] — Podman Desktop
- [[k3s]] — k3s 轻量级 Kubernetes
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- virtual-kubelet
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
