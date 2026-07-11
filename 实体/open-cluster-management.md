---
title: Open Cluster Management (OCM)
description: '## 概述'
summary: 'Open Cluster Management (OCM) 是一个社区驱动的多集群管理平台，提供 Kubernetes 多集群编排的核心能力。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。'
category: entities
tags:
- k8s
- cncf
- orchestration
- open-cluster-management
- prometheus
- grafana
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Cluster Management (OCM) 是什么
- 如何 Open Cluster Management (OCM)
trigger_keywords:
- Open
- Cluster
- Management
- OCM
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/open-cluster-management.md|Open Cluster Management]] (OCM)

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Open Cluster Management（OCM）是由 Red Hat 开源的多集群管理平台，2021 年加入 CNCF Sandbox。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。与其他多集群方案不同，OCM 设计了清晰的 Cluster API、Placement API 和 ManifestWork API，使多集群管理变得声明式和可扩展。它是 Red Hat Advanced Cluster Management（ACM）的开源上游项目。

## 核心特性

- **Hub-Spoke 架构**: Hub 集群集中管理，Klusterlet 代理注册到 Spoke 集群
- **集群注册**: ManagedCluster API 管理集群注册和状态上报
- **工作负载分发**: ManifestWork API 将 K8s 资源分发到托管集群
- **智能调度**: Placement API 支持按标签、拓扑、亲和性选择目标集群
- **策略治理**: Policy 框架支持配置合规检查和安全策略分发
- **Addon 框架**: 可扩展的 Addon 机制，支持自定义功能扩展

## 架构

OCM 采用 Hub-Agent 架构。Hub 集群运行 Registration Operator、Placement Controller 和 Cluster Manager。每个被管集群运行 Klusterlet（包含 Registration Agent 和 Work Agent）。Registration Agent 负责集群注册和证书管理；Work Agent 负责从 Hub 拉取 ManifestWork 并在本地集群应用。Placement Controller 根据 Placement 规则从 ManagedClusterSet 中选择目标集群。所有交互通过 CRD 声明式定义，Hub 不直接访问 Spoke 的 API Server，而是通过 ManifestWork 下发操作。

## Kubernetes 集成

OCM 完全基于 Kubernetes 原生 API 设计。ManagedCluster、ManagedClusterSet、Placement、ManifestWork 均为 CRD。Klusterlet 以 Deployment 形式部署在被管集群中，通过 Lease 机制保持心跳。Addon 框架允许第三方扩展（如observability addon）作为 Kubernetes Controller 运行。策略（Policy）框架通过 Gatekeeper/OPA 或自定义控制器实现合规检查。

## 生产使用场景

1. **多集群应用分发**: 将应用统一部署到开发、测试、生产集群
2. **策略合规管理**: 跨集群统一分发安全策略和配置基线
3. **边缘集群管理**: 管理大量边缘 Kubernetes 集群的生命周期
4. **灾难恢复**: 在多个集群间分发工作负载，实现故障切换

## 安装

```bash
# Hub 集群
clusteradm init --wait
# 注册 Spoke 集群
clusteradm join --hub-token <token> --hub-apiserver <url> --wait
# 或使用 Helm
helm install cluster-manager open-cluster-management/cluster-manager
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **OCM** | 轻量级、API 设计清晰、可扩展 | 社区较小、功能不如 ACM 丰富 |
| Karmada | 调度能力强、CNCF Incubating | 架构较重、学习曲线陡 |
| ArgoCD + ApplicationSet | GitOps 原生、成熟稳定 | 不是专门的多集群管理平台 |
| Clusternet | 支持边缘场景 | 社区更小 |

## 架构定位

在 CNCF 生态中，OCM 属于 **Orchestration** 类别，专注于多集群管理的标准化 API 设计。它是 Red Hat ACM 的上游，在企业级多集群管理领域占据重要位置。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[fluid]] — Fluid
- storage.md|cncf-storage]] — CNCF 存储与数据库项目全景
- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- open-cluster-management
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
