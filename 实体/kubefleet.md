---
title: KubeFleet [entities]
description: '## 概述'
summary: 'KubeFleet 是一个多集群资源编排平台，提供跨 Kubernetes 集群的工作负载分发、配置管理和策略驱动的资源放置能力。它通过 Hub-Member 架构和声明式 Placement 策略，实现将 Kubernetes 资源（Deployment、[[Service|Service]]、ConfigMap 等）自动分发到多个成员集群，'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubefleet
- cri-o
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
- KubeFleet 是什么
- 如何 KubeFleet
trigger_keywords:
- KubeFleet
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeFleet

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeFleet 是一个 CNCF 沙箱项目，由 Microsoft 开源，专注于 Kubernetes 多集群应用编排和资源调度。它提供统一的管理平面，将应用工作负载智能分发到多个集群，支持基于资源可用性、标签策略和地理位置的调度决策。KubeFleet 特别关注大规模边缘和混合云场景，解决多集群环境下的应用部署、配置管理和生命周期协调问题。项目是 Azure Kubernetes Fleet Manager 的开源核心。

## Key Features（核心能力）

- **多集群调度**：基于资源容量、标签约束和亲和性的智能工作负载调度
- **资源预留**：在目标集群预留资源确保部署成功
- **渐进式部署**：支持跨集群的滚动更新和金丝雀发布
- **配置传播**：跨集群的 ConfigMap、Secret、RBAC 等配置同步
- **集群分组**：通过集群属性（Property）和分组（ClusterGroup）管理集群
- **冲突解决**：自动处理多集群资源冲突和覆盖

## 架构与工作原理

KubeFleet 采用 Hub-Spoke 架构：Hub Cluster 运行 Fleet Manager 控制器，管理工作负载分发策略和集群状态；Member Clusters 运行 Fleet Agent，接收并执行分发指令。核心 CRD 包括 ClusterProperty（集群属性）、ClusterGroup（集群分组）、MemberCluster（成员集群注册）。调度引擎通过 Resource Distribution Controller 将工作负载按策略分发到目标集群，并跟踪各集群的部署状态。

## K8s 集成

KubeFleet 通过丰富的 CRD 与 Kubernetes 集成：MemberCluster CRD 注册成员集群；ClusterResourcePlacement CRD 定义资源分发策略（目标集群、调度约束、部署策略）；ClusterGroup CRD 定义集群分组。Hub Controller 通过各成员集群的 kubeconfig 连接到远程 API Server，推送配置和监控状态。Agent 在成员集群中协调实际资源创建。

## 生产用例

- **多集群应用部署**：将应用统一部署到多个生产集群
- **边缘计算编排**：将工作负载分发到地理分布的边缘集群
- **灾难恢复**：跨集群的工作负载快速迁移和恢复
- **多环境管理**：统一管理 dev/staging/prod 的应用部署

## 安装与快速开始

```bash
helm repo add kubefleet https://azure.github.io/fleet/charts
helm install fleet kubefleet/fleet-manager -n fleet-system --create-namespace
```

## 对比替代方案

相比 Karmada（CNCF 孵化），KubeFleet 更关注 Azure 生态但功能类似。相比 KubeStellar，KubeFleet 的调度策略更丰富但社区更小。

## Related

- [[cedar]] — Cedar
- [[cri-o]] — CRI-O
- [[shipwright]] — Shipwright
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubefleet
- [[实体/cncf-orchestration.md|[[CNCF 编排与应用管理项目全景|CNCF 编排与应用管理项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
