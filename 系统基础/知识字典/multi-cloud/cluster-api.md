---
title: Cluster API 集群生命周期
description: Cluster API（CAPI）是 CNCF 孵化项目，使用 Kubernetes 声明式 API 管理集群的生命周期（创建/升级/扩缩/删除），是声明式集群...
summary: Cluster API（CAPI）是 CNCF 孵化项目，使用 Kubernetes 声明式 API 管理集群的生命周期（创建/升级/扩缩/删除），是声明式集群...
category: dictionary
tags:
- k8s
- glossary
- multi-cloud
- lifecycle
- cncf
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cluster API 集群生命周期 是什么
- Cluster API 详解
trigger_keywords:
- Cluster API 集群生命周期
- Cluster API
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cluster API 集群生命周期（Cluster API）

## 概述

Cluster API（CAPI）是 CNCF 孵化项目，使用 Kubernetes 声明式 API 管理集群的生命周期（创建/升级/扩缩/删除），是声明式集群管理的标准框架。

## 核心概念/原理

- **声明式集群管理**：CRD 定义集群/机器/基础设施
- **Provider 模型**：基础设施/引导/控制平面 Provider
- **CNCF 孵化**：K8s SIG Cluster Lifecycle 核心项目
- **GitOps 友好**：集群状态纳入 Git 管理

## 关键机制或特性

- Cluster CRD 定义目标集群
- Machine/MachineSet/MachineDeployment 工作节点管理
- Infrastructure Provider（AWS/Azure/GCP/Docker/OpenStack）
- Bootstrap Provider（kubeadm/ignition）
- Control Plane Provider（kubeadm/K3s）
- ClusterClass 集群模板
- 自动化升级（滚动更新）

## 使用场景与最佳实践

- 大规模集群的声明式管理
- 多基础设施的集群自动化
- GitOps 式集群生命周期
- 集群的自动扩缩容
- 最佳实践：ClusterClass 标准化、Git 管理、渐进式升级

## 参考链接

- https://cluster-api.sigs.k8s.io/
- https://github.com/kubernetes-sigs/cluster-api

## Related

- [[系统基础/知识字典/fundamentals/cluster.md|Cluster]]
- [[系统基础/知识字典/tooling/kubeadm.md|kubeadm]]
- [[系统基础/知识字典/platform-engineering/kubestellar.md|KubeStellar]]


<!-- risk-assessed -->
