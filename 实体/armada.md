---
title: Armada (entities)
description: '## 概述'
summary: 'Armada 是一个多集群批处理作业调度系统，专为在多个 Kubernetes 集群上运行大规模批处理工作负载（如 HPC 计算、ML 训练、CI/CD 等）而设计。'
category: entities
tags:
- k8s
- cncf
- orchestration
- armada
- scheduler
- job
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Armada 是什么
- 如何 Armada
trigger_keywords:
- Armada
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Armada

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Armada 是一个多集群批处理作业调度系统，由 G-Research 开发，2023 年加入 CNCF 沙箱。它专为在多个 Kubernetes 集群上运行大规模批处理工作负载（如 HPC 计算、ML 训练、CI/CD 等）而设计。Armada 提供统一的作业提交入口、跨集群的公平调度（Fair Share）、优先级抢占和作业队列管理，能够管理数百万个并发作业在数千个节点上的高效调度。与 Kubernetes 原生的 Job/Deployment 调度不同，Armada 将多个物理 Kubernetes 集群抽象为一个大型计算池，用户提交作业到 Armada 队列，系统自动选择最优集群执行。它还提供了 Lookout UI 用于监控作业状态和队列积压。

## 核心能力

- **多集群调度**: 统一管理多个 Kubernetes 集群的计算资源
- **公平调度**: 基于权重的多租户 Fair Share 调度，防止资源独占
- **优先级抢占**: 高优先级作业可以抢占低优先级作业的资源
- **队列管理**: 按团队/项目划分队列，设置资源配额和优先级
- **JobSet 支持**: 将相关作业分组管理，支持 DAG 依赖
- **Lookout UI**: Web 界面监控队列状态、作业完成率和资源利用率

## 架构

Armada 采用控制面 + 执行面的双层架构：

- **Armada Server**: 控制面，接收作业提交、管理队列、执行调度算法
- **Queue System**: 基于 Redis/PostgreSQL 的作业队列存储，支持公平调度
- **Scheduler**: 核心调度引擎，根据 Fair Share 和优先级将作业分配到集群
- **Executor (Armada-executor)**: 部署在每个 Kubernetes 集群中的 Agent，接收作业并创建 Pod
- **Event Updater**: 收集作业执行状态，反馈到 Armada Server
- **Lookout**: Web UI 仪表板，展示作业和队列状态
- **Pulsar (可选)**: 用于大规模作业事件的流式消息

调度流程：`用户提交 → Queue → Scheduler (Fair Share) → Executor → K8s Pod → 状态回传`

## K8s 集成

Armada 的 Executor 以 Deployment 运行在每个目标 Kubernetes 集群中。Executor 接收来自 Armada Server 的作业指令，将其转换为 Kubernetes Pod/Job 资源创建到本地集群。Executor 监控 Pod 状态并实时回传到 Server。用户通过 `armadactl` CLI 或 REST API 提交 YAML 格式的作业定义（类似 Kubernetes Pod spec）。Armada 支持 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的全部 Pod 特性（resource requests、nodeSelector、tolerations 等），并与 JobSet API 集成支持复杂批处理工作流。

## 生产场景

1. **大规模 ML 训练**: 在多集群 GPU 池上调度分布式训练任务，实现 Fair Share
2. **金融量化计算**: G-Research 场景——数千个并行量化分析任务的多集群调度
3. **CI/CD 流水线**: 将构建/测试作业分发到多个集群，弹性利用闲置资源
4. **多租户计算平台**: 不同团队/项目通过队列隔离，Fair Share 分配集群资源

## 安装

```bash
# 安装 Armada Server
helm repo add armada https://armadaproject.io/charts/
helm install armada-server armada/server -n armada --create-namespace

# 在目标集群安装 Executor
helm install armada-executor armada/executor -n armada --create-namespace \
  --set armadaAddress=armada-server.armada.svc:50051

# 安装 CLI
brew install armadaproject/armada/armadactl

# 提交作业
armadactl submit ./job.yaml --queue my-team-queue

# 查看作业状态
armadactl queue watch my-team-queue
```

## 对比

| 特性 | Armada | Volcano | Kueue | YuniKorn |
|------|--------|---------|-------|----------|
| 多集群 | ✅ | ❌ | ⚠️ 部分 | ❌ |
| Fair Share | ✅ | ⚠️ 有限 | ✅ | ✅ |
| 批处理优化 | ✅ | ✅ | ✅ | ✅ |
| 队列管理 | ✅ | ⚠️ | ✅ | ✅ |

## 架构定位

在 CNCF 生态中，Armada 属于 **Orchestration** 类别，为云原生应用提供多集群批处理调度能力。

## 参考链接

- [[pod-lifecycle]]
- [[实体/kube-scheduler.md|kube-scheduler]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[keycloak]] — Keycloak
- [[kubearmor]] — KubeArmor
- [[实体/cncf-cicd.md|cncf-cicd]] — CNCF CI/CD 与发布管理项目全景
- networking.md|cncf-networking]] — CNCF 网络与服务网格项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- digest-2026-05-21-full
- 08-multicloud-federation-karmada
- armada
- karmada
- [[实体/cohdi.md|Cohdi]]
- [[实体/kubefleet.md|KubeFleet]]
- [[实体/clusternet.md|Clusternet]]
- [[实体/kured.md|Kured (KUbernetes REboot Daemon)]]
- [[实体/kubevela.md|KubeVela]]
- [[实体/kubestellar.md|KubeStellar]]
- [[实体/microcks.md|Microcks]]
- [[实体/kudo.md|KUDO]]
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
