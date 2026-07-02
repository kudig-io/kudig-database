---
title: Kubernetes
description: Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术...
summary: Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术...
category: dictionary
tags:
- k8s
- glossary
- kubernetes
- k8s
- container-orchestration
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 是什么
- Kubernetes (K8s) 详解
trigger_keywords:
- Kubernetes
- Kubernetes (K8s)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes

> **英文名**: Kubernetes (K8s)

## 概述

Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术栈的核心基础设施。

## 核心概念/原理

### 核心架构

```
┌─────────────────────────────────┐
│         Control Plane           │
│  ┌──────────┐  ┌─────────────┐  │
│  │apiserver │  │  scheduler  │  │
│  └──────────┘  └─────────────┘  │
│  ┌──────────────────────────┐   │
│  │  controller-manager      │   │
│  └──────────────────────────┘   │
│  ┌──────┐                       │
│  │ etcd │                       │
│  └──────┘                       │
└─────────────────────────────────┘
         ↕
┌─────────────────────────────────┐
│         Worker Nodes            │
│  ┌────────┐  ┌───────────────┐  │
│  │kubelet │  │  kube-proxy   │  │
│  └────────┘  └───────────────┘  │
│  ┌─────────────────────────┐    │
│  │  Container Runtime      │    │
│  └─────────────────────────┘    │
└─────────────────────────────────┘
```

### 声明式模型

Kubernetes 采用声明式 API：用户描述「期望状态」（Desired State），控制器持续将「实际状态」推向「期望状态」。

## 关键机制或特性

- **自动调度**：根据资源需求和约束将 Pod 调度到最佳节点。
- **自愈能力**：Pod 崩溃自动重启，节点故障自动迁移。
- **水平扩缩**：通过 HPA/VPA 自动调整资源。
- **服务发现与负载均衡**：通过 Service 和 Ingress 暴露应用。
- **滚动更新与回滚**：零停机部署和快速回滚。
- **声明式配置**：GitOps 友好的基础设施即代码。

## 使用场景与最佳实践

- 使用 kubeadm 初始化生产级集群。
- 遵循最小权限原则配置 RBAC。
- 为所有工作负载设置 resource requests/limits。
- 使用命名空间隔离不同团队或环境的工作负载。
- 启用审计日志（Audit Log）追踪 API 操作。
- 定期升级集群版本，关注弃用 API 迁移。

## 参考链接

- [Kubernetes Official Documentation](https://kubernetes.io/docs/)

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/node.md|Node]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace.md|Namespace]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster.md|Cluster]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cncf.md|CNCF]]


<!-- risk-assessed -->
