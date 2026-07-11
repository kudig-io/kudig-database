---
title: kcp
description: '## 概述'
summary: 'kcp 是一个类 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 服务器，提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kcp
- etcd
- rbac
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kcp 是什么
- 如何 kcp
trigger_keywords:
- kcp
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kcp

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

kcp 是一个类 Kubernetes API 服务器，由 Upbound 和 Red Hat 等团队推动开发，2021 年作为 CNCF 沙箱项目加入。它提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。kcp 利用 Kubernetes 的 API 机制（CRD、控制器、准入控制等），将其从容器编排中解耦出来，作为通用的 API 平台使用。kcp 支持在单个服务器上运行数千个逻辑集群（Workspace），每个 Workspace 拥有独立的 API 视图和资源隔离。这使得 kcp 非常适合作为 SaaS 平台的控制平面、多租户 API 服务或自定义控制器平台。

## 核心能力

- **逻辑集群（Workspace）**: 单进程内运行数千个逻辑集群，每个 Workspace 拥有独立的资源视图
- **APIExport/APIBinding**: 将自定义 API 跨 Workspace 共享和绑定，实现平台 API 的组合
- **多租户隔离**: 基于 Workspace 的强隔离，RBAC 细粒度权限控制
- **Syncer 机制**: 将逻辑集群中的资源同步到物理 Kubernetes 集群执行
- **标准 Kubernetes API**: 完全兼容 kubectl、client-go、CRD、Webhook 等原生工具链
- **轻量级**: 单二进制部署，无需完整控制面栈，适合嵌入式场景

## 架构

kcp 的核心架构围绕 "API as a Platform" 理念设计：

- **kcp server**: 单一进程，内嵌 etcd 和 Kubernetes API 服务器逻辑，管理所有 Workspace
- **Workspace**: 逻辑隔离单元，类似于虚拟集群，拥有独立的 Namespace 和资源
- **APIExport**: Workspace 声明可对外暴露的 API 资源（如自定义 CRD）
- **APIBinding**: 消费者 Workspace 绑定其他 Workspace 暴露的 API
- **Syncer**: 部署在物理集群中的 Agent，监听逻辑集群中的资源并同步到实际集群
- **Workload API**: 跨 Workspace 管理工作负载的生命周期

架构模式：`kcp (逻辑控制面) → Syncer → 物理 Kubernetes 集群 (执行面)`

## K8s 集成

kcp 本身就是一个精简版的 Kubernetes API 服务器，完全兼容 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 规范。通过 kubectl 和标准 client-go 库直接操作。Syncer 组件以 Deployment 方式部署在物理 Kubernetes 集群中，将 kcp 逻辑集群中的 Deployment、StatefulSet 等资源同步到实际集群运行。kcp 支持 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的所有原生资源类型，可通过 CRD 扩展自定义资源。

## 生产场景

1. **SaaS 平台控制面**: 为每个租户提供独立的逻辑集群，通过 APIExport 统一管理平台 API
2. **多团队开发平台**: 大型组织为每个团队分配独立 Workspace，集中管控但互不干扰
3. **自定义控制器平台**: 在 kcp 上运行业务控制器，无需完整 Kubernetes 集群
4. **混合云管理**: 通过 Syncer 将工作负载分发到不同云厂商的物理集群

## 安装

```bash
# 安装 kcp
kubectl krew install kcp
kcp start

# 或者从源码安装
git clone https://github.com/kcp-dev/kcp.git
cd kcp && make build
./bin/kcp start

# 配置 kubectl 连接 kcp
export KUBECONFIG=$(pwd)/.kcp/admin.kubeconfig
kubectl get workspaces
```

## 对比

| 特性 | kcp | vCluster | Capsule |
|------|-----|----------|---------|
| 隔离方式 | 逻辑 Workspace | 虚拟集群 | Namespace 聚合 |
| 原生 API | ✅ 完全兼容 | ✅ 完全兼容 | ⚠️ 共享 API |
| 无需物理集群 | ✅ 单进程 | ❌ 需宿主集群 | ❌ 需宿主集群 |
| 适合场景 | API 平台 | 开发测试 | 多租户隔离 |

## 架构定位

在 CNCF 生态中，kcp 属于 **Orchestration** 类别，为云原生应用提供关键的多租户 API 平台能力。

## 参考链接

- [[etcd]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[loxilb]] — LoxiLB
- [[kube-ovn]] — Kube-OVN
- [[flatcar]] — Flatcar Container Linuxux 生产环境速查卡|Linux]]
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kcp
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
